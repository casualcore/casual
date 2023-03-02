//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/handle.h"

#include "domain/message/discovery.h"
#include "domain/discovery/common.h"
#include "domain/discovery/admin/server.h"

#include "configuration/message.h"

#include "common/message/dispatch/handle.h"
#include "common/message/event.h"
#include "common/message/signal.h"
#include "common/event/listen.h"
#include "common/algorithm.h"
#include "common/algorithm/is.h"
#include "common/event/send.h"
#include "common/message/internal.h"
#include "common/algorithm/sorted.h"
#include "common/server/handle/call.h"
#include "common/communication/instance.h"
#include "common/signal/timer.h"

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
                  template< typename M>
                  auto multiplex( State& state, const strong::ipc::id& destination, const M& message)
                  {
                     state::metric::message::count::send( message);
                     return state.multiplex.send( destination, message);
                  }

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
                        if( auto correlation = send::multiplex( state, provider.process.ipc, message))
                           result.emplace_back( correlation, provider.process.pid);

                        return result;
                     });

                     log::line( verbose::log, "pending: ", result);
                     return result;
                  }

               } // send

               namespace collect::known
               {
                  //! send request to all with the "known" ability, if any.
                  auto requests( State& state)
                  {
                     Trace trace{ "discovery::handle::local::detail::collect::known::requests"};

                     return detail::send::requests( state, 
                        state.coordinate.known.empty_pendings(), 
                        state.providers.filter( state::provider::Ability::fetch_known),
                        message::discovery::fetch::known::Request{ common::process::handle()});
                  }

               } // collect::known

               namespace content::normalize
               {
                  auto request( const State& state, message::discovery::request::Content content)
                  {
                     // we need to translate from a possible route to the actual name
                     for( auto& name : content.services)
                        if( auto found = algorithm::find( state.service_name.to_origin, name))
                           name = found->second;

                     // make sure we keep the invariants
                     algorithm::container::sort::unique( content.services);

                     return content;
                  }

                  auto reply( const State& state, message::discovery::reply::Content content)
                  {
                      message::discovery::reply::Content result;
                      result.queues = std::move( content.queues);

                     // we need to translate back from the actual name to possible routes
                     for( auto& service : content.services)
                        if( auto found = algorithm::find( state.service_name.to_routes, service.name))
                           for( auto& route : found->second)
                           {
                              service.name = route;
                              result.services.push_back( service);
                           }
                        else
                           result.services.push_back( std::move( service));

                     // make sure we keep the invariants
                     algorithm::container::sort::unique( result.services);

                     return result;
                  }

                  auto reply( const State& state, message::discovery::request::Content content)
                  {
                     message::discovery::request::Content result;
                     result.queues = std::move( content.queues);

                     // we need to translate back from the actual name to possible routes
                     for( auto& name : content.services)
                        if( auto found = algorithm::find( state.service_name.to_routes, name))
                           algorithm::append( found->second, result.services);
                        else
                           result.services.push_back( std::move( name));

                     // make sure we keep the invariants
                     algorithm::container::sort::unique( result.services);

                     return result;
                  }

               } // content::normalize

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
                        if( detail::send::multiplex( state, message.process.ipc, common::message::reverse::type( message)))
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
                        detail::send::multiplex( state, message.process.ipc, common::message::reverse::type( message));
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
                        state.providers.filter( state::provider::Ability::discover), request);

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

                        detail::send::multiplex( state, destination.ipc, message);
                     });
                  };
               }

               namespace rediscovery
               {
                  auto request( State& state)
                  {
                     return [ &state]( const message::discovery::api::rediscovery::Request& message)
                     {
                        Trace trace{ "discovery::handle::local::api::rediscovery::request"};
                        local::handler::entry( message);

                        // fetch known and discover

                        state.coordinate.known( detail::collect::known::requests( state), [ &state, destination = extract::destination( message)]( auto&& replies, auto&& outcome)
                        {
                           Trace trace{ "discovery::handle::local::api::rediscovery::request known done"};
                           log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                           message::discovery::Request request{ process::handle()};
                           request.domain = common::domain::identity();

                           for( auto& reply : replies)
                              request.content += std::move( reply.content);

                           request.content = detail::content::normalize::request( state, std::move( request.content));

                           auto send_discoveries = [ &state]( auto& request)
                           {
                              return detail::send::requests( 
                                 state, 
                                 state.coordinate.discovery.empty_pendings(),  
                                 state.providers.filter( state::provider::Ability::discover), request);
                           };

                           state.coordinate.discovery( send_discoveries( request), [ &state, destination](  auto&& replies, auto&& outcome)
                           {
                              Trace trace{ "discovery::handle::local::api::rediscovery::request discovery done"};
                              log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                              // accumulate content
                              decltype( range::front( replies).content) content;

                              for( auto& reply : replies)
                                 content += std::move( reply.content);

                              message::discovery::api::rediscovery::Reply message;
                              message.correlation = destination.correlation;

                              auto transform_name = []( auto& value){ return value.name;};

                              message.content.queues = algorithm::transform( content.queues, transform_name);
                              message.content.services = algorithm::transform( content.services, transform_name);

                              message.content = detail::content::normalize::reply( state, std::move( message.content));
                                 
                              detail::send::multiplex( state, destination.ipc, message);

                           });
                        });
                     };
                  }
               } // rediscovery
            } // api

            namespace detail
            {
               void handle_internal_lookup( State& state, message::discovery::Request&& message)
               {
                  Trace trace{ "discovery::handle::local::detail::handle_internal_lookup"};

                  auto send_requests = []( auto& state, auto& message)
                  {
                     message::discovery::lookup::Request request{ common::process::handle()};
                     request.scope = decltype( request.scope)::internal;
                     request.content = std::move( message.content);

                     return detail::send::requests( state, state.coordinate.lookup.empty_pendings(), state.providers.filter( state::provider::Ability::lookup), request);
                  }; 

                  state.coordinate.lookup( send_requests( state, message), [ &state, destination = local::extract::destination( message)]( auto replies, auto outcome)
                  {
                     Trace trace{ "discovery::handle::local::detail::handle_internal_lookup coordinate lookup"};
                     log::line( verbose::log, "replies: ", replies);

                     message::discovery::Reply message;
                     message.correlation = destination.correlation;
                     message.domain = common::domain::identity();

                     for( auto& reply : replies)
                        message.content += std::move( reply.content);

                     log::line( verbose::log, "message: ", message);

                     detail::send::multiplex( state, destination.ipc, message);
                  });
               }

               void handle_extended_lookup( State& state, message::discovery::Request&& message)
               {
                  Trace trace{ "discovery::handle::local::detail::handle_extended_lookup"};

                  auto send_requests = []( auto& state, auto& message)
                  {
                     message::discovery::lookup::Request request{ common::process::handle()};
                     request.scope = decltype( request.scope)::extended;
                     request.content = std::move( message.content);

                     return detail::send::requests( state, state.coordinate.lookup.empty_pendings(), state.providers.filter( state::provider::Ability::lookup), request);
                  };

                  state.coordinate.lookup( send_requests( state, message), [ &state, destination = local::extract::destination( message)]( auto replies, auto outcome)
                  {
                     Trace trace{ "discovery::handle::local::detail::handle_extended_lookup coordinate lookup"};
                     log::line( verbose::log, "replies: ", replies);

                     message::discovery::reply::Content content;
                     message::discovery::request::Content absent;

                     for( auto& reply : replies)
                     {
                        content += std::move( reply.content);
                        absent += std::move( reply.absent);
                     }

                     if( ! absent)
                     {
                        // we found everything. Just reply

                        message::discovery::Reply message;
                        message.correlation = destination.correlation;
                        message.domain = common::domain::identity();
                        message.content = std::move( content);

                        log::line( verbose::log, "message: ", message);

                        detail::send::multiplex( state, destination.ipc, message);

                        return;
                     }

                     // we need to forward the discovery and try to find the absent..

                     auto send_requests = [ &state]( auto absent)
                     {
                        message::discovery::Request request{ process::handle()};
                        request.domain = common::domain::identity();
                        request.content = detail::content::normalize::request( state, std::move( absent));

                        return detail::send::requests( state, 
                           state.coordinate.discovery.empty_pendings(), 
                           state.providers.filter( state::provider::Ability::discover), 
                           request);
                     };

                     state.coordinate.discovery( send_requests( std::move( absent)), [ &state, content = std::move( content), destination]( auto replies, auto outcome)
                     { 
                        Trace trace{ "discovery::handle::local::request coordinate external"};
                        log::line( verbose::log, "replies: ", replies);

                        message::discovery::Reply message;
                        message.correlation = destination.correlation;
                        message.domain = common::domain::identity();
                        message.content = std::move( content);
                        
                        for( auto& reply : replies)
                           message.content += std::move( reply.content);
                        
                        message.content = detail::content::normalize::reply( state, std::move( message.content));

                        log::line( verbose::log, "message: ", message);
                        detail::send::multiplex( state, destination.ipc, message);
                     });
                  });

               }

            } // detail

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
                     detail::send::multiplex( state, message.process.ipc, common::message::reverse::type( message));
                     return;
                  }

                  if( message.directive == decltype( message.directive)::local)
                     detail::handle_internal_lookup( state, std::move( message));
                  else
                     detail::handle_extended_lookup( state, std::move( message));

               };
            }

            namespace lookup
            {
               auto reply( State& state)
               {
                  return [&state]( message::discovery::lookup::Reply&& message)
                  {
                     Trace trace{ "discovery::handle::local::internal::reply"};
                     local::handler::entry( message);

                     state.coordinate.lookup( std::move( message));
                  };
               }
            } // lookup

            auto reply( State& state)
            {
               return [&state]( message::discovery::Reply&& message)
               {
                  Trace trace{ "discovery::handle::local::reply"};
                  local::handler::entry( message);

                  state.coordinate.discovery( std::move( message));
         
               };
            }

            namespace fetch::known
            {
               auto reply( State& state)
               {
                  return [&state]( message::discovery::fetch::known::Reply&& message)
                  {
                     Trace trace{ "discovery::handle::local::known::reply"};
                     local::handler::entry( message);

                     state.coordinate.known( std::move( message));
                  };
               }
            } // fetch::known

            namespace topology
            {
               void apply( State& state)
               {
                  Trace trace{ "discovery::handle::local::topology::apply"};

                  if( ! state.accumulate.topology)
                     return;

                  auto [ explore, implicit] = state.accumulate.topology.extract();
               
                  state.coordinate.known( detail::collect::known::requests( state), [ &state, explore = std::move( explore), implicit = std::move( implicit)]( auto&& replies, auto&& outcome) mutable
                  {
                     Trace trace{ "discovery::handle::local::topology::direct::update known done"};
                     log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                     for( auto& reply : replies)
                        explore.content += std::move( reply.content);

                     log::line( verbose::log, "explore: ", explore);
                     log::line( verbose::log, "implicit: ", implicit);

                     if( explore.content)
                     {
                        // send to all external providers, if any
                        for( auto& provider : state.providers.filter( state::provider::Ability::discover))
                           detail::send::multiplex( state, provider.process.ipc, explore);
                     }

                     // always send the implicit topology update
                     for( auto& provider : state.providers.filter( state::provider::Ability::topology))
                        detail::send::multiplex( state, provider.process.ipc, implicit);
                  });
               }

               namespace implicit
               {
                  auto update( State& state)
                  {
                     //! Will be received only if a domain downstream got a new connection
                     //! (and the chain of inbounds are configured with _discovery forward_)
                     return [ &state]( message::discovery::topology::implicit::Update&& message)
                     {
                        Trace trace{ "discovery::handle::local::topology::implicit::update"};
                        local::handler::entry( message);

                        if( state.runlevel > decltype( state.runlevel())::running)
                           return;

                        // we might have already handled the topology update
                        if( algorithm::find( message.domains, common::domain::identity()))
                           return;

                        // accumulate for later....
                        state.accumulate.topology.add( std::move( message));
                     };
                  }
               } // implicit

               namespace direct
               {
                  auto update( State& state)
                  {
                     //! Will be received only if this domain got a new connection
                     return [ &state]( message::discovery::topology::direct::Update&& message)
                     {
                        Trace trace{ "discovery::handle::local::topology::direct::update"};
                        local::handler::entry( message);

                        if( state.runlevel > decltype( state.runlevel())::running)
                           return;

                        state.accumulate.topology.add( std::move( message));
                     };
                  }
               } // direct

            } // topology

            auto timeout( State& state)
            {
               return [ &state]( const common::message::signal::Timeout& message)
               {
                  Trace trace{ "gateway::group::outbound::local::internal::handle::timeout"};
                  log::line( verbose::log, "message: ", message);

                  topology::apply( state);
               };
            }


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

            namespace configuration::update
            {
               auto request( State& state)
               {
                  return [ &state]( const casual::configuration::message::update::Request& message)
                  {
                     Trace trace{ "discovery::handle::local::configuration::update::request"};
                     handle::configuration_update( state, message.model);

                     detail::send::multiplex( state, message.process.ipc, common::message::reverse::type( message));
                  };
               }

               
            } // configuration::update

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
                  Trace trace{ "discovery::handle::local::service::manager::lookup"};

                  // Need to lookup service-manager with _wait_, and when we get the reply
                  // we advertise our services.
                  auto send_request = []()
                  {
                     return common::communication::instance::lookup::request( common::communication::instance::identity::service::manager.id);
                  };

                  return [ &state, correlation = send_request()]( const common::message::domain::process::lookup::Reply& message)
                  {
                     Trace trace{ "discovery::handle::local::service::manager::lookup::reply"};
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

      void configuration_update( State& state, const casual::configuration::Model& model)
      {
         Trace trace{ "discovery::handle::configuration_update"};

         state.service_name.to_routes.clear();
         state.service_name.to_origin.clear();

         for( auto& service : model.service.services)
         {
            state.service_name.to_routes[ service.name] = service.routes;
            for( auto& route : service.routes)
               state.service_name.to_origin[ route] = service.name;
         }
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
            local::fetch::known::reply( state),
            local::request( state),
            local::lookup::reply( state),
            local::reply( state),
            local::topology::direct::update( state),
            local::topology::implicit::update( state),
            local::timeout( state),
            local::configuration::update::request( state),
            local::shutdown::request( state),
            local::service::manager::lookup::reply( state),
            local::server::Handle{ 
               admin::services( state)
            }
         };
      }

   } // domain::discovery::handle
} // casual