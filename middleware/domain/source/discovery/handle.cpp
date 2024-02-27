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
                           result.emplace_back( correlation, provider.process);

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
                     algorithm::container::sort::unique( content.queues);

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
                           algorithm::container::append( found->second, result.services);
                        else
                           result.services.push_back( std::move( name));

                     // make sure we keep the invariants
                     algorithm::container::sort::unique( result.services);

                     return result;
                  }

               } // content::normalize

               auto send_external_discovery_request( State& state, message::discovery::request::Content content, std::vector< state::accumulate::request::Reply> reply_destinations)
               {
                  Trace trace{ "discovery::handle::local:detail::send_external_discovery_request"};
                  
                  message::discovery::Request request{ common::process::handle()};
                  request.domain = common::domain::identity();
                  request.content = detail::content::normalize::request( state, std::move( content));

                  log::line( verbose::log, "state.providers: ", state.providers);

                  // send request to all with the discover ability, if any.
                  auto pending = detail::send::requests( state, state.coordinate.discovery.empty_pendings(), 
                     state.providers.filter( state::provider::Ability::discover), request);

                  // note: everything captured needs to by value (besides State if used)
                  state.coordinate.discovery( std::move( pending), [ &state, reply_destinations = std::move( reply_destinations)]( auto replies, auto outcome) mutable
                  {
                     Trace trace{ "discovery::handle::local:detail::send_external_discovery_request coordinate.discovery"};
                     log::line( verbose::log, "reply_destinations: ", reply_destinations);
                     
                     auto content = algorithm::accumulate( replies, message::discovery::reply::Content{}, []( auto result, auto& reply)
                     {
                        return result + std::move( reply.content);
                     });

                     // Normalize to take routes into account
                     content = content::normalize::reply( state, std::move( content));
                     auto [ discoveries, api ] = algorithm::partition( reply_destinations, predicate::value::equal( state::accumulate::request::reply::Type::discovery));

                     // handle the different replies

                     if( discoveries)
                     {
                        message::discovery::Reply reply;
                        reply.domain = common::domain::identity();

                        for( auto& destination : discoveries)
                        {
                           // filter based on the requested resources.
                           reply.content = state.in_flight_cache.filter_reply( destination.correlation, content);
                           reply.correlation = destination.correlation;
                           detail::send::multiplex( state, destination.ipc, reply);
                        };
                     }
                     
                     if( api)
                     {
                        message::discovery::api::Reply reply;

                        for( auto& destination : api)
                        {
                           // filter based on the requested resources.
                           reply.content = state.in_flight_cache.filter_reply( destination.correlation, content);
                           reply.correlation = destination.correlation;
                           detail::send::multiplex( state, destination.ipc, reply);
                        };
                     }                     
                  });
               }

               // Just helpers to help use the function above
               auto send_external_discovery_request( State& state, message::discovery::Request request)
               {
                  send_external_discovery_request( state, std::move( request.content), { { state::accumulate::request::reply::Type::discovery, request.correlation, request.process.ipc}});
               }

               auto send_external_discovery_request( State& state, message::discovery::api::Request request)
               {
                  send_external_discovery_request( state, std::move( request.content), { { state::accumulate::request::reply::Type::api, request.correlation, request.process.ipc}});
               }

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

                        // send reply and register
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

                     state.in_flight_cache.add( message);

                     // Should we accumulate the request for later?
                     if( state.accumulate.bypass())
                        detail::send_external_discovery_request( state, std::move( message));
                     else
                        state.accumulate.add( std::move( message));

                     
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

                           // complement the request with potential in-flight request resources, and normalize to respect routes.
                           request.content = detail::content::normalize::request( state, state.in_flight_cache.complement( std::move( request.content)));

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

                     message::discovery::reply::Content known;
                     message::discovery::request::Content absent;

                     for( auto& reply : replies)
                     {
                        known += std::move( reply.content);
                        absent += std::move( reply.absent);
                     }

                     if( ! absent)
                     {
                        // we found everything. Just reply

                        message::discovery::Reply message;
                        message.correlation = destination.correlation;
                        message.domain = common::domain::identity();
                        message.content = std::move( known);

                        log::line( verbose::log, "message: ", message);

                        detail::send::multiplex( state, destination.ipc, message);
                        return;
                     }

                     // we need to ask potential other domains for resources. If we've got
                     // "local" known stuff, we need to cache this to union with the upcoming 
                     // reply.
                     if( known)
                        state.in_flight_cache.add_known( destination.correlation, std::move( known));


                     // "reconstruct" the request to either send directly, or accumulate for later.
                     message::discovery::Request request;
                     request.domain = common::domain::identity();
                     request.directive = decltype( request.directive)::forward;
                     request.correlation = destination.correlation;
                     request.process.ipc = destination.ipc;            
                     request.content = std::move( absent);

                     state.in_flight_cache.add( request);

                     if( state.accumulate.bypass())
                        detail::send_external_discovery_request( state, std::move( request));
                     else
                        state.accumulate.add( std::move( request));
                  });
               }

            } // detail

            auto request( State& state)
            {
               //! this is a request that comes from other domains, i.e. from our inbounds.
               return [ &state]( message::discovery::Request&& message)
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

                  // If there is only a local lookup, we do a "lot" less.
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

            namespace accumulate
            {
               namespace topology
               { 
                  void send_direct_explore( State& state, message::discovery::topology::direct::Explore direct, std::vector< common::domain::Identity> implicit_domains)
                  {
                     Trace trace{ "discovery::handle::local::accumulate::topology::extraction"};
                     log::line( verbose::log, "direct: ", direct, ", implicit_domains: ", implicit_domains);

                     state.coordinate.known( detail::collect::known::requests( state), [ &state, direct = std::move( direct), implicit_domains = std::move( implicit_domains)]( auto&& replies, auto&& outcome) mutable
                     {
                        Trace trace{ "discovery::handle::local::topology::direct::update known done"};
                        log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                        for( auto& reply : replies)
                           direct.content += std::move( reply.content);

                        // add the possible in-flight requests, that we've not got replies for yet. And normalize to respect routes.
                        direct.content = detail::content::normalize::request( state, state.in_flight_cache.complement( std::move( direct.content)));

                        log::line( verbose::log, "direct: ", direct);

                        if( direct.content)
                        {
                           // send to all external providers, if any
                           for( auto& provider : state.providers.filter( state::provider::Ability::discover))
                              detail::send::multiplex( state, provider.process.ipc, direct);
                        }

                        // always send the implicit topology update
                        message::discovery::topology::implicit::Update implicit;
                        implicit.domains = std::move( implicit_domains);
                        algorithm::append_unique_value( common::domain::identity(), implicit.domains);

                        for( auto& provider : state.providers.filter( state::provider::Ability::topology))
                           detail::send::multiplex( state, provider.process.ipc, implicit);
                     });

                  }

                  void extraction( State& state, state::accumulate::topology::extract::Result result)
                  {
                     Trace trace{ "discovery::handle::local::accumulate::topology::extraction"};

                     topology::send_direct_explore( state, std::move( result.direct), std::move( result.implicit_domains));
                  }

                } // topology

                namespace request
                {
                  void extraction( State& state, state::accumulate::request::extract::Result result)
                  {
                     Trace trace{ "discovery::handle::local::accumulate::request::extraction"};

                     detail::send_external_discovery_request( state, std::move( result.content), std::move( result.replies));
                  }
                   
                } // request



               void timeout( State& state)
               {
                  Trace trace{ "discovery::handle::local::accumulate::timeout"};

                  auto result = state.accumulate.extract();
                  log::line( verbose::log, "result: ", result);

                  if( result.topology)
                     accumulate::topology::extraction( state, std::move( *result.topology));

                  if( result.request)
                     accumulate::request::extraction( state, std::move( *result.request));

               }
               
            } // accumulate

            namespace topology
            {
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

                        if( ! state.accumulate.bypass())
                        {
                           // accumulate for later....
                           state.accumulate.add( std::move( message));
                           return;
                        }

                        // we need to send it directly
                        message::discovery::topology::direct::Explore direct;
                        direct.domains.push_back( message.origin);

                        accumulate::topology::send_direct_explore( state, std::move( direct), std::move( message.domains));

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

                        if( ! state.accumulate.bypass())
                        {
                           // accumulate for later....
                           state.accumulate.add( std::move( message));
                           return;
                        }

                        // we need to send it directly
                        message::discovery::topology::direct::Explore direct;
                        direct.domains.push_back( message.origin);
                        direct.content = std::move( message.configured);

                        // send_direct_explore will add our self to implicit domains for upstream domains,
                        // there are no other domains involved yet, since we're the one that got the _direct connection_
                        accumulate::topology::send_direct_explore( state, std::move( direct), {});
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

                  accumulate::timeout( state);
               };
            }


            namespace event
            {
               namespace process
               {
                  auto exit( State& state)
                  {
                     return [ &state]( const common::message::event::process::Exit& event)
                     {
                        Trace trace{ "discovery::handle::local::event::process::exit"};
                        log::line( verbose::log, "event: ", event);

                        state.failed( event.state.pid);
                     };
                  }
               } // process

               namespace ipc
               {
                  auto destroyed( State& state)
                  {
                     return [ &state]( const common::message::event::ipc::Destroyed& event)
                     {
                        Trace trace{ "discovery::handle::local::event::process::exit"};
                        log::line( verbose::log, "event: ", event);

                        state.failed( event.process.ipc);
                     };

                  }
               } // ipc

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
            common::event::listener( 
               local::event::process::exit( state),
               local::event::ipc::destroyed( state)),
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