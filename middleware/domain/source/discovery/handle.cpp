//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/handle.h"

#include "domain/message/discovery.h"
#include "domain/discovery/common.h"

#include "common/message/handle.h"
#include "common/message/event.h"
#include "common/event/listen.h"
#include "common/algorithm.h"
#include "common/event/send.h"
#include "common/communication/ipc/flush/send.h"

#include "casual/assert.h"

namespace casual
{
   using namespace common;

   namespace domain::discovery::handle
   {
      namespace local
      {
         namespace
         {

            namespace extract
            {
               struct Destination
               {
                  strong::ipc::id ipc;
                  strong::correlation::id correlation;

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( ipc);
                     CASUAL_SERIALIZE( correlation);
                  )
               };

               //! extracts and replaces process and correlation for the message
               //! @return the destination
               template< typename M>
               Destination destination( M& message)
               {
                  if constexpr( std::is_const_v< M>)
                     return { message.process.ipc, message.correlation};
                  else
                     return { std::exchange( message.process, common::process::handle()).ipc, std::exchange( message.correlation, {})};
               }
            } // extract


            namespace detail
            {
               namespace send
               {
                  template< typename Result, typename P, typename M>
                  auto requests( State& state, Result result, P&& providers, const M& message)
                  {
                     Trace trace{ "discovery::handle::local::detail::send::requests"};
                     log::line( verbose::log, "providers: ", providers);
                     casual::assertion( ! message.correlation);
                     casual::assertion( message.process == process::handle());

                     log::line( verbose::log, "message: ", message);

                     result = algorithm::accumulate( providers, std::move( result), [ &state, &message]( auto result, const auto& provider)
                     {
                        if( auto correlation = state.multiplex.send( provider.process.ipc, message))
                           result.emplace_back( correlation, provider.process.pid);

                        return result;
                     });

                     log::line( verbose::log, "pending: ", result);
                     return result;
                  }

               } // send

               namespace normalize
               {
                  void content( message::discovery::reply::Content& content)
                  {
                     auto equal_name = []( auto& l, auto& r)
                     {
                        return l.name == r.name;
                     };

                     auto service_order = []( auto& l, auto& r)
                     {
                        auto tie = []( auto& value){ return std::tie( value.name, value.type, value.transaction);};
                        return tie( l) < tie( r);
                     };

                     algorithm::container::trim( content.services, algorithm::unique( algorithm::sort( content.services, service_order), equal_name));


                     auto queue_order = []( auto& l, auto& r)
                     {
                        auto tie = []( auto& value){ return std::tie( value.name, value.retries);};
                        return tie( l) < tie( r);
                     };

                     algorithm::container::trim( content.queues, algorithm::unique( algorithm::sort( content.queues, queue_order), equal_name));

                  }
               } // normalize

               namespace collect::needs::then
               {
                  //! the `continuation` is invoked with all the discovery replies
                  template< typename F>
                  auto discover( State& state, F continuation)
                  {
                     Trace trace{ "discovery::handle::local::detail::collect::needs::then::discover"};

                     // send request to all with the discover ability, if any.
                     auto pending = detail::send::requests( state, state.coordinate.needs.empty_pendings(), 
                        state.providers.filter( state::provider::Ability::needs), message::discovery::needs::Request{ process::handle()});

                     // note: everything captured needs to by value (besides State if used)
                     state.coordinate.needs( std::move( pending), [ &state, continuation = std::move( continuation)]( auto replies, auto outcome)
                     {
                        Trace trace{ "discovery::handle::local::detail::collect::needs::then::discover collect-needs"};
                        log::line( verbose::log, "replies: ", replies, "outcome: ", outcome);
                        
                        // coordinate discovery requests..
                        message::discovery::Request request{ process::handle()};
                        for( auto& reply : replies)
                           request.content += std::move( reply.content);

                        if( ! request.content)
                        {
                           // Noting to discover, we let caller continuation do it's thing...
                           state.coordinate.discovery( {}, std::move( continuation));
                        }
                        else
                        {
                           auto pending = detail::send::requests( state, state.coordinate.discovery.empty_pendings(), 
                              state.providers.filter( state::provider::Ability::discover_external), request);

                           // let caller continuation do it's thing...
                           state.coordinate.discovery( std::move( pending), std::move( continuation));
                        }
                     });
                  }
               } // collect::needs::then

            } // detail


            namespace api
            {
               namespace provider
               {
                  auto registration( State& state)
                  {
                     return [&state]( const message::discovery::api::provider::registration::Request& message)
                     {
                        Trace trace{ "discovery::handle::local::provider::registration"};
                        log::line( verbose::log, "message: ", message);

                        // send reply and registrate
                        if( state.multiplex.send( message.process.ipc, common::message::reverse::type( message)))
                           state.providers.registration( message);
                     };
                  }
               } // provider

               auto request( State& state)
               {
                  return [&state]( message::discovery::api::Request&& message)
                  {
                     Trace trace{ "discovery::handle::local::api::request"};
                     log::line( verbose::log, "message: ", message);

                     if( state.runlevel > decltype( state.runlevel())::running || ! message.content)
                     {
                        // we don't take any more request, or the request is empty
                        state.multiplex.send( message.process.ipc, common::message::reverse::type( message));
                        return;
                     }

                     // mutate the message and extract 'reply destination'
                     auto destination = local::extract::destination( message);

                     message::discovery::Request request{ process::handle()};
                     request.content = std::move( message.content);
                     request.domain = common::domain::identity();

                     log::line( verbose::log, "state.providers: ", state.providers);

                     // send request to all with the discover ability, if any.
                     auto pending = detail::send::requests( state, state.coordinate.discovery.empty_pendings(), 
                        state.providers.filter( state::provider::Ability::discover_external), request);

                     // note: everything captured needs to by value (besides State if used)
                     state.coordinate.discovery( std::move( pending), [ &state, destination]( auto replies, auto outcome)
                     {
                        Trace trace{ "discovery::handle::local::api::request coordinate"};

                        message::discovery::api::Reply message;
                        message.correlation = destination.correlation;

                        state.multiplex.send( destination.ipc, message);
                     });
                  };
               }

               namespace rediscovery
               {
                  auto request( State& state)
                  {
                     return [ &state]( const message::discovery::api::rediscovery::Request& message)
                     {
                        Trace trace{ "discovery::handle::local::rediscovery::request"};
                        log::line( verbose::log, "message: ", message);

                        // mutate the message and extract 'reply destination'
                        auto destination = local::extract::destination( message);

                        // collect needs and discover then use our continuation
                        detail::collect::needs::then::discover( state, [ &state, destination = std::move( destination)]( auto replies, auto outcome)
                        {
                           Trace trace{ "discovery::handle::local::rediscovery::request continuation"};
                           log::line( verbose::log, "replies: ", replies, "outcome: ", outcome);

                           message::discovery::api::rediscovery::Reply reply;
                           reply.correlation = destination.correlation;
                           reply.content = algorithm::accumulate( replies, std::move( reply.content), []( auto result, auto& reply)
                           {
                              auto transform_names = []( auto& range){ return algorithm::transform( range, []( auto& value){ return value.name;});};

                              algorithm::append_unique( transform_names( reply.content.services), result.services);
                              algorithm::append_unique( transform_names( reply.content.queues), result.queues);

                              return result;
                           });

                           state.multiplex.send( destination.ipc, reply);
                        });
                     };
                  }
               } // rediscovery
            } // api

            namespace needs
            {
               auto reply( State& state)
               {
                  return [&state]( message::discovery::needs::Reply&& message)
                  {
                     Trace trace{ "discovery::handle::local::needs::reply"};
                     log::line( verbose::log, "message: ", message);

                     state.coordinate.needs( std::move( message));
                  };
               }
            } // needs

            auto request( State& state)
            {
               return [&state]( message::discovery::Request&& message)
               {
                  Trace trace{ "discovery::handle::local::request"};
                  log::line( verbose::log, "message: ", message);

                  if( state.runlevel > decltype( state.runlevel())::running)
                  {
                     // we don't take any more request.
                     state.multiplex.send( message.process.ipc, common::message::reverse::type( message));
                     return;
                  }

                  auto handle_request = []( State& state, auto&& range, message::discovery::Request& message)
                  {
                     // mutate the message and extract 'reply destination'
                     auto destination = local::extract::destination( message);
                     log::line( verbose::log, "destination: ", destination);

                     auto pending = detail::send::requests( state, state.coordinate.discovery.empty_pendings(), range, message);

                     // note: everything captured needs to by value (besides State if used)
                     state.coordinate.discovery( std::move( pending), [ &state, destination]( auto replies, auto outcome)
                     {
                        Trace trace{ "discovery::handle::local::request coordinate"};

                        message::discovery::Reply message{ process::handle()};
                        message.correlation = destination.correlation;

                        for( auto& reply : replies)
                           message.content += std::move( reply.content);

                        local::detail::normalize::content( message.content);
                        log::line( verbose::log, "message: ", message);

                        state.multiplex.send( destination.ipc, message);
                     });
                  };

                  using Ability = state::provider::Ability;

                  // should we ask all 'agents' or only the local inbounds
                  if( message.directive == decltype( message.directive)::forward)
                     handle_request( state, state.providers.filter( { Ability::discover_external, Ability::discover_internal}), message);
                  else
                     handle_request( state, state.providers.filter( Ability::discover_internal), message);
               };
            }

            auto reply( State& state)
            {
               return [&state]( message::discovery::Reply&& message)
               {
                  Trace trace{ "discovery::handle::local::reply"};
                  log::line( verbose::log, "message: ", message);

                  state.coordinate.discovery( std::move( message));
         
               };
            }

            namespace topology
            {
               auto update( State& state)
               {
                  return [ &state]( message::discovery::topology::Update&& message)
                  {
                     Trace trace{ "discovery::handle::local::topology::update"};
                     log::line( verbose::log, "message: ", message);

                     if( state.runlevel > decltype( state.runlevel())::running)
                        return;

                     // we might have already handled the topology update
                     if( algorithm::find( message.domains, common::domain::identity()))
                        return;

                     message.domains.push_back( common::domain::identity());

                     // collect needs and discover then use our continuation (mutable to enable move from captured)
                     detail::collect::needs::then::discover( state, [ &state, domains = std::move( message.domains)]( auto replies, auto outcome) mutable
                     {
                        Trace trace{ "discovery::handle::local::topology::update coordinate continuation"};
                        log::line( verbose::log, "replies: ", replies, "outcome: ", outcome);

                        // we're not interested in the replies, we just propagate the topology::Update to 
                        // all with that ability, if any.

                        message::discovery::topology::Update message;
                        message.domains = std::move( domains);

                        for( auto& provider : state.providers.filter( state::provider::Ability::topology))
                           state.multiplex.send( provider.process.ipc, message);
                     });
                  };
               }
            } // topology

            namespace event
            {
               namespace process
               {
                  auto exit( State& state)
                  {
                     return [&state]( const common::message::event::process::Exit& event)
                     {
                        Trace trace{ "discovery::handle::local::event::process::exit"};
                        log::line( verbose::log, "event: ", event);

                        state.providers.remove( event.state.pid);
                        state.coordinate.failed( event.state.pid);
                     };
                  }
               } // process

            } // event

            namespace shutdown
            {
               auto request( State& state)
               {
                  return [&state]( const common::message::shutdown::Request& message)
                  {
                     Trace trace{ "discovery::handle::local::shutdown::request"};
                     log::line( verbose::log, "message: ", message);

                     state.runlevel = decltype( state.runlevel())::shutdown;
                  };
               }
            } // shutdown

         } // <unnamed>
      } // local

      dispatch_type create( State& state)
      {
         return {
            common::message::handle::defaults( common::communication::ipc::inbound::device()),
            common::event::listener( local::event::process::exit( state)),
            local::api::provider::registration( state),
            local::api::request( state),
            local::api::rediscovery::request( state),
            local::needs::reply( state),
            local::request( state),
            local::reply( state),
            local::topology::update( state),
            local::shutdown::request( state)
         };
      }

   } // domain::discovery::handle
} // casual