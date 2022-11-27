//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/handle.h"

#include "domain/message/discovery.h"
#include "domain/discovery/common.h"
#include "domain/discovery/admin/server.h"

#include "common/message/dispatch/handle.h"
#include "common/message/event.h"
#include "common/event/listen.h"
#include "common/algorithm.h"
#include "common/algorithm/is.h"
#include "common/event/send.h"
#include "common/message/internal.h"
#include "common/algorithm/sorted.h"
#include "common/server/handle/call.h"
#include "common/communication/instance.h"

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
            namespace handler
            {
               template< typename M>
               void entry( const M& message)
               {
                  log::line( verbose::log, "message: ", message);
                  state::metric::message::count::receive( message);
               }
            
            } // handler

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

                     CASUAL_ASSERT( ! message.correlation);
                     CASUAL_ASSERT( message.process == process::handle());

                     log::line( verbose::log, "message: ", message);

                     result = algorithm::accumulate( providers, std::move( result), [ &state, &message]( auto result, const auto& provider)
                     {
                        if( auto correlation = state.multiplex.send( provider.process.ipc, message))
                        {
                           state::metric::message::count::send( message);
                           result.emplace_back( correlation, provider.process.pid);
                        }

                        return result;
                     });

                     log::line( verbose::log, "pending: ", result);
                     return result;
                  }

               } // send

               namespace collect
               {
                  namespace helper
                  {
                     //! just a helper for the discovery phase
                     template< typename F>
                     auto discover( State& state, F continuation, message::discovery::request::Content content = {})
                     {
                        // note: everything captured needs to be by value (besides State if used)
                        return [ &state, continuation = std::move( continuation), content = std::move( content)]( auto replies, auto outcome) mutable
                        {
                           Trace trace{ "discovery::handle::local::detail::collect::helper::discover"};
                           log::line( verbose::log, "content: ", content, ", replies: ", replies, ", outcome: ", outcome);
                           
                           // coordinate discovery requests..
                           message::discovery::Request request{ process::handle()};
                           request.content = std::move( content);
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
                        };
                     }

                  } // helper

                  namespace needs::then
                  {
                     //! the `continuation` is invoked with all the discovery replies
                     template< typename F>
                     auto discover( State& state, F continuation)
                     {
                        Trace trace{ "discovery::handle::local::detail::collect::needs::then::discover"};

                        // send request to all with the discover ability, if any.
                        auto pending = detail::send::requests( state, state.coordinate.needs.empty_pendings(), 
                           state.providers.filter( state::provider::Ability::needs), message::discovery::needs::Request{ common::process::handle()});

                        state.coordinate.needs( std::move( pending), collect::helper::discover( state, std::move( continuation)));
                     }
                  } // needs::then

                  namespace known::then
                  {
                     //! the `continuation` is invoked with all the discovery replies
                     template< typename F>
                     auto discover( State& state, F continuation, message::discovery::request::Content content)
                     {
                        Trace trace{ "discovery::handle::local::detail::collect::known::then::discover"};

                        // send request to all with the discover ability, if any.
                        auto pending = detail::send::requests( state, state.coordinate.known.empty_pendings(), 
                           state.providers.filter( state::provider::Ability::known), message::discovery::known::Request{ common::process::handle()});

                        // note: everything captured needs to by value (besides State if used)
                        state.coordinate.known( std::move( pending), collect::helper::discover( state, std::move( continuation), std::move( content)));
                     }
                  } // known::then

               } // collect

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
                        local::handler::entry( message);

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
                     local::handler::entry( message);

                     if( state.runlevel > decltype( state.runlevel())::running || ! message.content)
                     {
                        // we don't take any more request, or the request is empty
                        state.multiplex.send( message.process.ipc, common::message::reverse::type( message));
                        return;
                     }

                     // mutate the message and extract 'reply destination'
                     auto destination = local::extract::destination( message);

                     message::discovery::Request request{ common::process::handle()};
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
                        message.content = algorithm::accumulate( replies, message.content, []( auto result, auto& reply)
                        {
                           return result + std::move( reply.content);
                        });

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
                        local::handler::entry( message);

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

                              return result + decltype( result){
                                 transform_names( reply.content.services),
                                 transform_names( reply.content.queues)};
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
                     local::handler::entry( message);

                     state.coordinate.needs( std::move( message));
                  };
               }
            } // needs

            namespace known
            {
               auto reply( State& state)
               {
                  return [&state]( message::discovery::known::Reply&& message)
                  {
                     Trace trace{ "discovery::handle::local::known::reply"};
                     local::handler::entry( message);

                     state.coordinate.known( std::move( message));
                  };
               }
            } // known

            namespace detail
            {
               namespace content::normalize
               {
                  auto request( message::discovery::request::Content request, const message::discovery::reply::Content& reply, const message::discovery::internal::reply::service::Routes& routes)
                  {
                     Trace trace{ "discovery::handle::detail::request::normalize::content"};

                     algorithm::container::trim( request.services, std::get< 1>( algorithm::sorted::intersection( request.services, reply.services)));
                     algorithm::container::trim( request.queues, std::get< 1>( algorithm::sorted::intersection( request.queues, reply.queues)));

                     // make sure to "map" routes to the origin names (only services for now)
                     for( auto& service : request.services)
                        if( auto found = routes.find_name( service))
                           service = found->origin;

                     // make sure we keep the invariants
                     algorithm::container::sort::unique( request.services);

                     return request;
                  }

                  auto reply( message::discovery::reply::Content reply, const message::discovery::internal::reply::service::Routes& routes)
                  {
                     // make sure to "map" back routes to the name (only services for now)
                     for( auto& service : reply.services)
                        if( auto found = routes.find_origin( service.name))
                           service.name = found->name;

                     // make sure we keep the invariants
                     algorithm::container::sort::unique( reply.services);
                     
                     return reply;
                  }
               } // content::normalize
            } // local

            auto request( State& state)
            {
               return [&state]( message::discovery::Request&& message)
               {
                  Trace trace{ "discovery::handle::local::request"};
                  local::handler::entry( message);

                  // assert preconditions
                  CASUAL_ASSERT( algorithm::is::sorted( message.content.services) && algorithm::is::unique( message.content.services));
                  CASUAL_ASSERT( algorithm::is::sorted( message.content.queues) && algorithm::is::unique( message.content.queues));

                  // We need to coordinate a few things
                  //  * fetch all internal services/queues along with possible _routes_, from the request
                  //  * remove services/queues that we have internally, from the "external request"
                  //  * from the remaining "not found internally", map routes, if any.
                  //  * from this we fetch all external (with the original names for routes)
                  //  * aggregate the replies, and map "original names" back to the route names
                  //  * add this with the internal 
                  //  * send reply.
                   

                  if( state.runlevel > decltype( state.runlevel())::running)
                  {
                     // we don't take any more request.
                     state.multiplex.send( message.process.ipc, common::message::reverse::type( message));
                     return;
                  }

                  // first we call the internal providers, to get the _local stuff_ and possible routes
                  auto internal_providers = state.providers.filter( state::provider::Ability::discover_internal);
                  message::discovery::internal::Request request{ common::process::handle()};
                  request.content = message.content;
                  auto pending = detail::send::requests( state, state.coordinate.internal.empty_pendings(), internal_providers, request);

                  // if we going to "forward" the discovery, we do a two step coordination, first internal, and 
                  // we use that information for the external coordination. 
                  if( message.directive == decltype( message.directive)::forward)
                  {
                     state.coordinate.internal( std::move( pending), [ &state, message = std::move( message)]( auto replies, auto outcome) mutable
                     {
                        Trace trace{ "discovery::handle::local::request coordinate internal"};
                        log::line( verbose::log, "replies: ", replies);

                        auto content = algorithm::accumulate( replies, message::discovery::reply::Content{}, []( auto result, auto& reply)
                        {
                           return result += std::move( reply.content);
                        });

                        auto routes = algorithm::accumulate( replies, message::discovery::internal::reply::service::Routes{}, []( auto result, auto& reply)
                        {
                           return result += std::move( reply.routes);
                        });

                        // mutate the message content, for possible use with _external_ discovery
                        message.content = detail::content::normalize::request( std::move( message.content), content, routes);

                        if( ! message.content)
                        {
                           log::line( verbose::log, "only internal discovery was needed");
                           auto reply = common::message::reverse::type( message);
                           reply.content = std::move( content);
                           state.multiplex.send( message.process.ipc, reply);

                           return;
                        }

                        auto destination = local::extract::destination( message);
                        auto pending = detail::send::requests( state, state.coordinate.discovery.empty_pendings(), state.providers.filter( state::provider::Ability::discover_external), message);

                        // we need to ask our "external provider"
                        state.coordinate.discovery( std::move( pending), [ &state, destination, content = std::move( content), routes = std::move( routes)]( auto replies, auto outcome)
                        { 
                           Trace trace{ "discovery::handle::local::request coordinate external"};
                           log::line( verbose::log, "replies: ", replies);

                           message::discovery::Reply message;
                           message.correlation = destination.correlation;
                           message.content = std::move( content);
                           
                           for( auto& reply : replies)
                              message.content += std::move( reply.content);
                           
                           if( routes)
                              message.content = detail::content::normalize::reply( std::move( message.content), routes);

                           log::line( verbose::log, "message: ", message);

                           state.multiplex.send( destination.ipc, message);
                        });
                     });
                  }
                  else
                  {
                     // we don't ask our "external provider", we can reply directly
                     state.coordinate.internal( std::move( pending), [ &state, destination = local::extract::destination( message)]( auto replies, auto outcome)
                     {
                        Trace trace{ "discovery::handle::local::request coordinate internal"};
                        log::line( verbose::log, "replies: ", replies);

                        message::discovery::Reply message;
                        message.correlation = destination.correlation;

                        for( auto& reply : replies)
                           message.content += std::move( reply.content);

                        log::line( verbose::log, "message: ", message);

                        state.multiplex.send( destination.ipc, message);
                     });
                  }
               };
            }

            namespace internal
            {
               auto reply( State& state)
               {
                  return [&state]( message::discovery::internal::Reply&& message)
                  {
                     Trace trace{ "discovery::handle::local::internal::reply"};
                     local::handler::entry( message);

                     state.coordinate.internal( std::move( message));
                  };
               }
            } // internal

            auto reply( State& state)
            {
               return [&state]( message::discovery::Reply&& message)
               {
                  Trace trace{ "discovery::handle::local::reply"};
                  local::handler::entry( message);

                  state.coordinate.discovery( std::move( message));
         
               };
            }

            namespace topology
            {
               namespace upstream
               {
                  void apply( State& state)
                  {
                     Trace trace{ "discovery::handle::local::topology::upstream::apply"};

                     message::discovery::topology::implicit::Update message;
                     message.domains = state.accumulate.upstream.extract();
                     log::line( verbose::log, "message: ", message);

                     for( auto& provider : state.providers.filter( state::provider::Ability::topology))
                        state.multiplex.send( provider.process.ipc, message);
                  }
               } // upstream

               namespace implicit
               {
                  void apply( State& state)
                  {
                     Trace trace{ "discovery::handle::local::topology::implicit::apply"};

                     // collect needs from this domain and discover - then use our continuation and propagate the 
                     // topology update _upstream_
                     detail::collect::needs::then::discover( state, [ &state, domains = state.accumulate.implicit.extract()]( auto&& replies, auto&& outcome)
                     {
                        Trace trace{ "discovery::handle::local::topology::implicit continuation"};

                        // we're not interested in the replies, we add and accumulate for upstream
                        state.accumulate.upstream.add( std::move( domains));
                        
                        if( state.accumulate.upstream.limit())
                           topology::upstream::apply( state);
                     });
                  }

                  auto update( State& state)
                  {
                     //! Will be received only if a domain downstream got a new connection
                     //! (and the chain of inbounds are configured with _discovery forward_)
                     return [ &state]( message::discovery::topology::implicit::Update&& message)
                     {
                        Trace trace{ "discovery::handle::local::topology::update"};
                        local::handler::entry( message);

                        if( state.runlevel > decltype( state.runlevel())::running)
                           return;

                        // we might have already handled the topology update
                        if( algorithm::find( message.domains, common::domain::identity()))
                           return;

                        // accumulate for later....
                        state.accumulate.implicit.add( std::move( message.domains));

                        // check if we've reach the "accumulate limit" and need to apply.
                        if( state.accumulate.implicit.limit())
                           topology::implicit::apply( state);
                     };
                  }
               } // implicit

               namespace direct
               {
                  void apply( State& state)
                  {
                     Trace trace{ "discovery::handle::local::topology::direct::apply"};

                     // collect known from this domain and discover - then use our continuation and propagate the 
                     // topology update _upstream_
                     detail::collect::known::then::discover( state, [ &state]( auto&& replies, auto&& outcome)
                     {
                        Trace trace{ "discovery::handle::local::topology::apply coordinate continuation"};

                        // we're not interested in the replies, we just propagate the topology::Update to 
                        // all with that ability, if any.

                        // we're not interested in the replies, we add and accumulate for upstream
                        state.accumulate.upstream.add( { common::domain::identity()});
                        
                        if( state.accumulate.upstream.limit())
                           topology::upstream::apply( state);

                     }, state.accumulate.direct.extract());
                  }

                  auto update( State& state)
                  {
                     //! Will be received only if this domain got a new connection
                     return [ &state]( message::discovery::topology::direct::Update&& message)
                     {
                        Trace trace{ "discovery::handle::local::topology::direct::update"};
                        local::handler::entry( message);

                        if( state.runlevel > decltype( state.runlevel())::running)
                           return;

                        state.accumulate.direct.add( std::move( message.content));

                        // check if we've reach the "accumulate limit" and need to apply.
                        if( state.accumulate.direct.limit())
                           topology::direct::apply( state);
                     };
                  }
               } // direct

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
                     local::handler::entry( message);

                     state.runlevel = decltype( state.runlevel())::shutdown;
                  };
               }
            } // shutdown

            namespace service::manager::lookup
            {
               auto reply( State& state)
               {
                  Trace trace{ "discovery::handle::local::process::lookup::reply create"};

                  // Need to lookup service-manager with _wait_, and when we get the reply
                  // we advertise our services.
                  auto send_request = []()
                  {
                     return common::communication::instance::lookup::request( common::communication::instance::identity::service::manager.id);
                  };

                  return [ &state, correlation = send_request()]( const common::message::domain::process::lookup::Reply& message)
                  {
                     Trace trace{ "discovery::handle::local::process::lookup::reply"};
                     log::line( verbose::log, "message: ", message);

                     if( message.correlation == correlation)
                     {
                        casual::assertion( communication::instance::identity::service::manager == message.identification, "message.identification is not service-manager ",  message.identification);
                        log::line( verbose::log, "service-manager is online");

                        common::server::handle::policy::advertise( admin::services( state).services);
                     }
                  };

               }
            } // service::manager::lookup

            namespace server
            {
               using base_type = common::server::handle::policy::call::Admin;
               struct Policy : base_type
               {
                  void configure( common::server::Arguments&& arguments)
                  {
                     // no-op, we'll advertise our services when the service-manager comes online.
                  }
               };

               using Handle = common::server::handle::basic_call< Policy>;
            } // server

         } // <unnamed>
      } // local

      void idle( State& state)
      {
         Trace trace{ "discovery::handle::idle"};

         if( state.runlevel > decltype( state.runlevel())::running)
            return;

         if( state.accumulate.direct)
            local::topology::direct::apply( state);

         if( state.accumulate.implicit)
            local::topology::implicit::apply( state);

         if( state.accumulate.upstream)
            local::topology::upstream::apply( state);

      }

      dispatch_type create( State& state)
      {
         Trace trace{ "discovery::handle::create"};



         return dispatch_type{
            common::message::dispatch::handle::defaults( state),
            common::event::listener( local::event::process::exit( state)),
            local::api::provider::registration( state),
            local::api::request( state),
            local::api::rediscovery::request( state),
            local::needs::reply( state),
            local::known::reply( state),
            local::request( state),
            local::internal::reply( state),
            local::reply( state),
            local::topology::direct::update( state),
            local::topology::implicit::update( state),
            local::shutdown::request( state),
            local::service::manager::lookup::reply( state),
            local::server::Handle{ 
               admin::services( state)
            }
         };
      }

   } // domain::discovery::handle
} // casual