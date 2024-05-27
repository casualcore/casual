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
               }
            
            } // handler

            namespace detail
            {
               namespace send
               {
                  template< typename M>
                  auto multiplex( State& state, const strong::ipc::id& destination, const M& message)
                  {
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

               } // content::normalize

               namespace content
               {
                  bool includes( const message::discovery::request::Content& request, const message::discovery::reply::Content& reply)
                  {
                     return algorithm::includes( reply.services, request.services)
                        && algorithm::includes( reply.queues, request.queues);
                  };

                  //! @returns the difference between a and b. a - b
                  message::discovery::request::Content difference( message::discovery::request::Content a, const message::discovery::reply::Content& b)
                  {
                     auto erase_if = []( auto& container, auto& lookup)
                     {
                        algorithm::container::erase_if( container, [ &lookup]( auto& value)
                        {
                           return predicate::boolean( algorithm::find( lookup, value));
                        });
                     };

                     erase_if( a.services, b.services);
                     erase_if( a.queues, b.queues);

                     return a;
                  }

                  auto send_reply( State& state, const message::discovery::reply::Content& content, auto& request)
                  {
                     auto filter_content = []( auto& request_content, auto& reply_content)
                     {
                        return algorithm::accumulate( request_content, decltype( reply_content){}, [ &reply_content]( auto result, auto& request)
                        {
                           if( auto found = algorithm::find( reply_content, request))
                              result.push_back( *found);
                           return result;
                        });
                     };
                     
                     auto reply = common::message::reverse::type( request);
                     reply.content.services = filter_content( request.content.services, content.services);
                     reply.content.queues = filter_content( request.content.queues, content.queues);

                     detail::send::multiplex( state, request.process.ipc, reply);
                  }

                  auto send_reply_if_included( State& state, const message::discovery::reply::Content& content, auto& request)
                  { 
                     if( ! content::includes( request.content, content))
                        return false;

                     send_reply( state, content, request);
                     return true;
                     
                  };

               } // content

               void send_aggregated_discovery( State& state, 
                  std::vector< message::discovery::Request> discovery, 
                  std::vector< message::discovery::api::Request> api,
                  message::discovery::reply::Content local_content = {})
               {
                  Trace trace{ "discovery::handle::local:detail::send_aggregated_discovery"};

                  if( discovery.empty() && api.empty())
                     return;

                  message::discovery::Request request{ common::process::handle()};
                  request.domain = common::domain::identity();

                  request.content = [ &]()
                  {
                     auto accumulate_content = []( auto result, auto& value)
                     {
                        return result + value.content;   
                     };

                     auto content = algorithm::accumulate( discovery, message::discovery::request::Content{}, accumulate_content);
                     return algorithm::accumulate( api, std::move( content), accumulate_content);
                  }();

                  // remove all service/queues that was found locally. We don't want to discover stuff that is
                  // "local"
                  request.content = detail::content::difference( std::move( request.content), local_content);
                  request.content = detail::content::normalize::request( state, std::move( request.content));

                  auto pendings = [ &]()
                  {
                     // no point of discovery with no content to discover...
                     if( ! request.content)
                        return typename decltype( state.coordinate.discovery)::pendings_type{};

                     return detail::send::requests( state, typename decltype( state.coordinate.discovery)::pendings_type{}, 
                        state.providers.filter( state::provider::Ability::discover), request);
                  }();


                  auto remove_guard = execute::scope( [ &state, correlation = state.pending_content.add( std::move( request.content))]()
                  {
                     state.pending_content.remove( correlation);
                  });

                  struct Shared
                  {
                     message::discovery::reply::Content content;
                     std::vector< message::discovery::Request> discovery;
                     std::vector< message::discovery::api::Request> api;
                  };

                  auto shared = std::make_shared< Shared>();
                  shared->content = std::move( local_content);
                  shared->discovery = std::move( discovery);
                  shared->api = std::move( api);


                  using Directive = common::message::coordinate::minimal::fan::out::Directive;

                  auto done_callback = [ &state, shared, remove_guard = std::move( remove_guard)]( auto failed)
                  {
                     Trace trace{ "discovery::handle::local:detail::send_aggregated_discovery done_callback"};

                     auto send_reply = [ &state, &content = shared->content]( auto& request)
                     {
                        content::send_reply( state, content, request);
                     };

                     algorithm::for_each( shared->api, send_reply);
                     algorithm::for_each( shared->discovery, send_reply);
                  };

                  auto message_callback = [ &state, shared]( message::discovery::Reply reply)
                  {
                     Trace trace{ "discovery::handle::local:detail::send_aggregated_discovery message_callback"};

                     shared->content += detail::content::normalize::reply( state, std::move( reply.content));

                     auto send_if_included = [ &]( auto& request)
                     {
                        return content::send_reply_if_included( state, shared->content, request);
                     };

                     algorithm::container::erase_if( shared->api, send_if_included);
                     algorithm::container::erase_if( shared->discovery, send_if_included);

                     if( shared->api.empty() && shared->discovery.empty())
                        return Directive::done;
                     else
                        return Directive::pending;
                  };

                  state.coordinate.discovery.add( std::move( pendings), std::move( message_callback), std::move( done_callback));
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
                        detail::send::multiplex( state, message.process.ipc, common::message::reverse::type( message));
                        return;
                     }

                     if( state.accumulate)
                        state.accumulate.add( std::move( message));
                     else
                        detail::send_aggregated_discovery( state, {}, { std::move( message)});
                     
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

                        state.coordinate.known( detail::collect::known::requests( state), [ &state, message]( auto&& replies, auto&& outcome)
                        {
                           Trace trace{ "discovery::handle::local::api::rediscovery::request known done"};
                           log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                           auto pendings = [ &]()
                           {
                              message::discovery::Request request{ process::handle()};
                              request.domain = common::domain::identity();

                              for( auto& reply : replies)
                                 request.content += std::move( reply.content);

                              // normalize to respect routes.
                              request.content = detail::content::normalize::request( state, std::move( request.content));

                              // we add the possible in-flight request content for good measure.
                              request.content += state.pending_content();

                              return detail::send::requests( 
                                 state, 
                                 typename decltype( state.coordinate.discovery)::pendings_type{},  
                                 state.providers.filter( state::provider::Ability::discover), request);

                           }();

                           auto shared = std::make_shared< message::discovery::api::rediscovery::Reply>();
                           *shared = common::message::reverse::type( message);

                           auto message_callback = [ shared]( auto& reply)
                           {
                              Trace trace{ "discovery::handle::local::api::rediscovery::request message_callback"};

                              shared->content += std::move( reply.content);

                              // we'll wait for all replies.
                              return common::message::coordinate::minimal::fan::out::Directive::pending;
                           };

                           auto done_callback = [ &state, shared, ipc = message.process.ipc]( auto failed)
                           {
                               Trace trace{ "discovery::handle::local::api::rediscovery::request done_callback"};

                              detail::send::multiplex( state, ipc, *shared);
                           };

                           state.coordinate.discovery.add( std::move( pendings), message_callback, done_callback);
                           
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

                  auto pendings = [ &]()
                  {
                     message::discovery::lookup::Request request{ common::process::handle()};
                     request.scope = decltype( request.scope)::internal;
                     request.content = std::move( message.content);

                     return detail::send::requests( state, state.coordinate.lookup.empty_pendings(), state.providers.filter( state::provider::Ability::lookup), request);
                  }(); 

                  state.coordinate.lookup( std::move( pendings), [ &state, request = std::move( message)]( auto replies, auto outcome)
                  {
                     Trace trace{ "discovery::handle::local::detail::handle_internal_lookup coordinate lookup"};
                     log::line( verbose::log, "replies: ", replies);

                     auto message = common::message::reverse::type( request);
                     message.domain = common::domain::identity();

                     for( auto& reply : replies)
                        message.content += std::move( reply.content);

                     log::line( verbose::log, "message: ", message);

                     detail::send::multiplex( state, request.process.ipc, message);
                  });
               }

               void handle_extended_lookup( State& state, message::discovery::Request request)
               {
                  Trace trace{ "discovery::handle::local::detail::handle_extended_lookup"};

                  message::discovery::lookup::Request lookup{ common::process::handle()};
                  lookup.scope = decltype( lookup.scope)::extended;
                  // borrow the content 
                  lookup.content = std::move( request.content);

                  auto pendings = detail::send::requests( state, 
                     state.coordinate.lookup.empty_pendings(), 
                     state.providers.filter( state::provider::Ability::lookup), 
                     lookup);

                  request.content = std::move( lookup.content);

                  state.coordinate.lookup( std::move( pendings), [ &state, request = std::move( request)]( auto replies, auto outcome)
                  {
                     Trace trace{ "discovery::handle::local::detail::handle_extended_lookup coordinate lookup"};
                     log::line( verbose::log, "replies: ", replies);

                     auto lookup_content = algorithm::accumulate( replies, message::discovery::reply::Content{}, []( auto result, auto& reply)
                     {
                        return result + reply.content;
                     });

                     if( content::send_reply_if_included( state, lookup_content, request))
                        return;

                     // should we do the external discovery later
                     if( state.accumulate)
                     {
                        state.accumulate.add( std::move( request));

                        // we keep the lookup-content to get possible local services/queues
                        state.accumulate.add( std::move( lookup_content));
                        return;
                     }

                     // we discover directly
                     detail::send_aggregated_discovery( state, { std::move( request)}, {}, std::move( lookup_content));

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


                  if( state.runlevel > decltype( state.runlevel())::running)
                  {
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

                  log::line( verbose::log, "state.coordinate.discovery: ", state.coordinate.discovery);
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
                  void send_direct_explore( State& state, 
                     std::vector< message::discovery::topology::direct::Update> direct, 
                     std::vector< message::discovery::topology::implicit::Update> implicit)
                  {
                     Trace trace{ "discovery::handle::local::accumulate::topology::send_direct_explore"};
                     log::line( verbose::log, "direct: ", direct, ", implicit: ", implicit);

                     if( direct.empty() && implicit.empty())
                        return;

                     state.coordinate.known( detail::collect::known::requests( state), [ &state, direct = std::move( direct), implicit = std::move( implicit)]( auto&& replies, auto&& outcome) mutable
                     {
                        Trace trace{ "discovery::handle::local::accumulate::topology::send_direct_explore coordinate"};
                        log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                        message::discovery::topology::direct::Explore explore;
                        
                        for( auto& reply : replies)
                           explore.content += std::move( reply.content);

                        for( auto& message : direct)
                           explore.content += std::move( message.configured);

                        explore.content = detail::content::normalize::request( state, std::move( explore.content));

                        // we add content for possible pending discovery request, to make sure we get to know all possible
                        // services/queues that the new connection might provide
                        explore.content += state.pending_content();

                        log::line( verbose::log, "explore: ", explore);

                        if( explore.content)
                        {
                           // we only send to the specific ipc that sent the topology update to
                           // begin with. These we know has topology changes
                           // We unique them, since it could be several topology updates for a
                           // given "connection"

                           auto transform_ipc = []( auto& value){ return value.process.ipc;};

                           auto targets = algorithm::transform( direct, transform_ipc);
                           algorithm::transform( implicit, targets, transform_ipc);

                           for( auto& target : algorithm::unique( algorithm::sort( targets)))
                              detail::send::multiplex( state, target, explore);
                        }

                        // always send the implicit topology update upstream
                        message::discovery::topology::implicit::Update message;

                        // accumulate all downstream domains that has triggered us. This is only used to terminate a 
                        // a possible "topology ring", that will otherwise run forever...
                        message.domains = algorithm::accumulate( implicit, message.domains, []( auto result, auto& message)
                        {
                           algorithm::append_unique( message.domains, result);
                           return result;
                        });

                        algorithm::append_unique_value( common::domain::identity(), message.domains);

                        for( auto& provider : state.providers.filter( state::provider::Ability::topology))
                           detail::send::multiplex( state, provider.process.ipc, message);
                     });

                  }

                } // topology
               
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

                        // we always accumulate topology
                        state.accumulate.add( std::move( message));
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

                        // we always accumulate topology
                        state.accumulate.add( std::move( message));

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

                  auto result = state.accumulate.extract();
                  log::line( verbose::log, "result: ", result);

                  accumulate::topology::send_direct_explore( state, std::move( result.direct), std::move( result.implicit));
                  detail::send_aggregated_discovery( state, std::move( result.discovery), std::move( result.api), std::move( result.lookup));
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