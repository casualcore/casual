//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/server/handle/policy.h"

#include "common/transaction/context.h"
#include "common/buffer/pool.h"
#include "common/service/lookup.h"
#include "common/communication/instance.h"

#include "common/message/domain.h"

#include "common/log.h"
#include "common/execute.h"

namespace casual
{
   namespace common
   {
      namespace server
      {
         namespace handle
         {
            namespace policy
            {
               namespace local
               {
                  namespace
                  {
                     void advertise( std::vector< message::service::advertise::Service> services)
                     {
                        Trace trace{ "common::server::local::advertise"};

                        if( ! services.empty())
                        {
                           message::service::Advertise advertise;
                           advertise.process = process::handle();
                           advertise.services = std::move( services);

                           log::line( log::debug, "advertise: ", advertise);

                           signal::thread::scope::Mask block{ signal::set::filled( signal::Type::terminate, signal::Type::interrupt)};

                           communication::ipc::blocking::send( communication::instance::outbound::service::manager::device(), advertise);
                        }
                     }

                     namespace configure
                     {
                        void services(
                           std::vector< server::Service> services,
                           message::domain::configuration::server::Reply::Service configuration)
                        {
                           Trace trace{ "common::server::local::configure::services"};

                           auto equal_service_name = []( auto& lhs, auto& rhs){ return lhs.name == rhs.name;};

                           // if this server has restrictions, we intersect.
                           if( ! configuration.restrictions.empty())
                              algorithm::trim( services, std::get< 0>( algorithm::intersection( services, configuration.restrictions)));

                           // split the ones that has routes, and those who have not
                           auto split = algorithm::intersection( services, configuration.routes, equal_service_name);

                           // transform the non-routes directly (the complement of the intersection)
                           auto advertise = algorithm::transform( std::get< 1>( split), []( auto& service)
                           {
                              return message::service::advertise::Service{ service.name, service.category, service.transaction};
                           });
                           
                           auto handle_route = [&]( auto& service)
                           {
                              auto physical = server::context().physical( service.name);
                              assert( physical);

                              auto route = algorithm::find_if( configuration.routes, [&]( auto& route){ return route.name == service.name;});

                              auto transform_route = [physical,&service]( auto& route)
                              {
                                 // bind the physical service to the configured route.
                                 server::Context::instance().state().services.emplace( route, *physical);
                                 return message::service::advertise::Service{ route, service.category, service.transaction};
                              };

                              algorithm::transform( route->routes, advertise, transform_route);
                           };

                           // handle the configured routes, if any.
                           auto& routes = std::get< 0>( split);
                           algorithm::for_each( routes, handle_route);

                           local::advertise( std::move( advertise));
                        }

                     } // configure

                     message::domain::configuration::server::Reply configuration()
                     {
                        message::domain::configuration::server::Request request;
                        request.process = process::handle();

                        return communication::ipc::call( communication::instance::outbound::domain::manager::device(), request);
                     }

                  } // <unnamed>
               } // local

               namespace call
               {

                  void Default::configure( server::Arguments& arguments)
                  {
                     Trace trace{ "server::handle::policy::Default::configure"};

                     // Connection to the domain has been done before...

                     // Ask domain-manager for our configuration
                     auto configuration = policy::local::configuration();

                     // configure resources, if any.
                     transaction::Context::instance().configure( arguments.resources, std::move( configuration.resources));

                     // Let the service-manager know about our services...
                     policy::local::configure::services( arguments.services, std::move( configuration.service));
                  }

                  void Default::reply( strong::ipc::id id, message::service::call::Reply& message)
                  {
                     Trace trace{ "server::handle::policy::Default::reply"};

                     log::line( log::debug, "reply: ", message);

                     communication::ipc::blocking::send( id, message);
                  }

                  void Default::reply( strong::ipc::id id, message::conversation::callee::Send& message)
                  {
                     Trace trace{ "server::handle::policy::Default::conversation::reply"};

                     log::line( log::debug, "reply: ", message);

                     auto node = message.route.next();
                     communication::ipc::blocking::send( node.address, message);
                  }

                  void Default::ack( const message::service::call::ACK& message)
                  {
                     Trace trace{ "server::handle::policy::Default::ack"};

                     log::line( verbose::log, "reply: ", message);

                     communication::ipc::blocking::send( communication::instance::outbound::service::manager::device(), message);
                  }

                  void Default::statistics( strong::ipc::id id,  message::event::service::Call& event)
                  {
                     Trace trace{ "server::handle::policy::Default::statistics"};

                     log::line( log::debug, "event:", event);

                     try
                     {
                        communication::ipc::blocking::send( id, event);
                     }
                     catch( ...)
                     {
                        exception::handle();
                     }
                  }

                  void Default::transaction(
                        const common::transaction::ID& trid,
                        const server::Service& service,
                        const common::platform::time::unit& timeout,
                        const platform::time::point::type& now)
                  {
                     Trace trace{ "server::handle::policy::Default::transaction"};

                     log::line( log::debug, "trid: ", trid, " - service: ", service);

                     // We keep track of callers transaction (can be null-trid).
                     transaction::context().caller = trid;

                     switch( service.transaction)
                     {
                        case service::transaction::Type::automatic:
                        {
                           if( trid)
                              transaction::Context::instance().join( trid);
                           else
                              transaction::Context::instance().start( now);

                           break;
                        }
                        case service::transaction::Type::branch:
                        {
                           if( trid)
                              transaction::Context::instance().branch( trid);
                           else
                              transaction::Context::instance().start( now);
                           break;
                        }
                        case service::transaction::Type::join:
                        {
                           transaction::Context::instance().join( trid);
                           break;
                        }
                        case service::transaction::Type::atomic:
                        {
                           transaction::Context::instance().start( now);
                           break;
                        }
                        default:
                        {
                           log::line( log::category::error, "unknown transaction semantics for service: ", service);
                           // fallthrough
                        }
                        case service::transaction::Type::none:
                        {
                           // We don't start or join any transactions
                           // (technically we join a null-trid)
                           transaction::Context::instance().join( transaction::ID{ process::handle()});
                           break;
                        }
                     }

                     // Set 'global deadline'
                     transaction::Context::instance().current().timout.set( now, timeout);
                  }


                  message::service::Transaction Default::transaction( bool commit)
                  {
                     return transaction::context().finalize( commit);
                  }


                  namespace local
                  {
                     namespace
                     {
                        template< typename M>
                        void forward( M&& message, service::invoke::Forward&& forward)
                        {
                           Trace trace{ "server::handle::policy::local::forward"};


                           if( transaction::context().pending())
                           {
                              throw common::exception::xatmi::service::Error( "service: " + message.service.name + " tried to forward with pending transactions");
                           }

                           common::service::Lookup lookup{
                              forward.parameter.service.name,
                              common::service::Lookup::Context::forward};


                           auto request = message;

                           auto target = lookup();

                           if( target.busy())
                           {
                              // We wait for service to become idle
                              target = lookup();
                           }

                           request.buffer = std::move( forward.parameter.payload);
                           request.service = target.service;

                           log::line( log::debug, "policy::Default::forward - request:", request);

                           communication::ipc::blocking::send( target.process.ipc, request);
                        }

                     } // <unnamed>
                  } // local

                  void Default::forward( service::invoke::Forward&& forward, const message::service::call::callee::Request& message)
                  {
                     local::forward( message, std::move( forward));
                  }


                  void Default::forward( service::invoke::Forward&& forward, const message::conversation::connect::callee::Request& message)
                  {
                     local::forward( message, std::move( forward));
                  }


                  Admin::Admin( communication::error::type handler)
                     : m_error_handler{ std::move( handler)}
                  {}


                  void Admin::configure( server::Arguments& arguments)
                  {
                     // Connection to the domain has been done before...

                     if( ! arguments.resources.empty())
                     {
                        throw exception::system::invalid::Argument{ "can't build and link an administration server with resources"};
                     }

                     policy::local::configure::services( arguments.services, {});

                  }

                  void Admin::reply( strong::ipc::id id, message::service::call::Reply& message)
                  {
                     communication::ipc::blocking::send( id, message, m_error_handler);
                  }

                  void Admin::ack( const message::service::call::ACK& message)
                  {
                     communication::ipc::blocking::send( communication::instance::outbound::service::manager::device(), message, m_error_handler);
                  }


                  void Admin::statistics( strong::ipc::id id, message::event::service::Call& event)
                  {
                     // no-op
                  }

                  void Admin::transaction(
                        const common::transaction::ID& trid,
                        const server::Service& service,
                        const common::platform::time::unit& timeout,
                        const platform::time::point::type& now)
                  {
                     // no-op
                  }

                  message::service::Transaction Admin::transaction( bool commit)
                  {
                     // no-op
                     return {};
                  }

                  void Admin::forward( common::service::invoke::Forward&& forward, const message::service::call::callee::Request& message)
                  {
                     throw common::exception::xatmi::System{ "can't forward within an administration server"};
                  }

               } // call

            } // policy

         } // handle
      } // server
   } // common
} // casual
