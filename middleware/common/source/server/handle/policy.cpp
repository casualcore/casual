//!
//! casual 
//!

#include "common/server/handle/policy.h"

#include "common/transaction/context.h"
#include "common/buffer/pool.h"
#include "common/service/lookup.h"
#include "common/communication/ipc.h"

#include "common/message/domain.h"



#include "common/log.h"

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

                           log::debug << "advertise: " << advertise << '\n';

                           communication::ipc::blocking::send( communication::ipc::broker::device(), advertise);
                        }
                     }

                     namespace configure
                     {
                        void services(
                              const std::vector< server::Service>& services,
                              const message::domain::configuration::server::Reply& configuration)
                        {
                           Trace trace{ "common::server::local::configure::services"};

                           std::vector< message::service::advertise::Service> advertise;


                           for( auto& service : services)
                           {

                              auto physical = server::Context::instance().physical( service.origin);


                              if( physical && ( configuration.restrictions.empty() || range::find( configuration.restrictions, service.origin)))
                              {
                                 auto found = range::find_if( configuration.routes, [&service]( const message::domain::configuration::server::Reply::Service& s){
                                    return s.name == service.origin;
                                 });

                                 if( found)
                                 {
                                    range::for_each( found->routes, [physical,&advertise,&service]( const std::string& name){

                                       advertise.emplace_back( name, service.category, service.transaction);

                                       server::Context::instance().state().services.emplace( name, *physical);

                                    });

                                 }
                                 else
                                 {
                                    advertise.emplace_back( service.origin, service.category, service.transaction);
                                 }
                              }
                           }

                           local::advertise( std::move( advertise));
                        }

                     } // configure

                     message::domain::configuration::server::Reply configuration()
                     {
                        message::domain::configuration::server::Request request;
                        request.process = process::handle();

                        return communication::ipc::call( communication::ipc::domain::manager::device(), request);
                     }

                  } // <unnamed>
               } // local

               namespace call
               {

                  void Default::configure( server::Arguments& arguments)
                  {
                     Trace trace{ "server::handle::policy::Default::configure"};

                     //
                     // Connection to the domain has been done before...
                     //

                     //
                     // Ask domain-manager for our configuration
                     //
                     auto configuration = policy::local::configuration();

                     //
                     // Let the broker know about our services...
                     //
                     policy::local::configure::services( arguments.services, configuration);

                     //
                     // configure resources, if any.
                     //
                     transaction::Context::instance().configure( arguments.resources, std::move( configuration.resources));

                  }

                  void Default::reply( platform::ipc::id::type id, message::service::call::Reply& message)
                  {
                     Trace trace{ "server::handle::policy::Default::reply"};

                     communication::ipc::blocking::send( id, message);
                  }

                  void Default::reply( platform::ipc::id::type id, message::conversation::caller::Send& message)
                  {
                     Trace trace{ "server::handle::policy::Default::conversation::reply"};

                     auto node = message.route.next();
                     communication::ipc::blocking::send( node.address, message);
                  }

                  void Default::ack( const message::service::call::callee::Request& message)
                  {
                     Trace trace{ "server::handle::policy::Default::ack"};

                     message::service::call::ACK ack;
                     ack.process = process::handle();
                     ack.service = message.service.name;

                     communication::ipc::blocking::send( communication::ipc::broker::device(), ack);
                  }

                  void Default::ack( const message::conversation::connect::callee::Request& message)
                  {
                     Trace trace{ "server::handle::policy::Default::ack"};

                     message::service::call::ACK ack;
                     ack.process = process::handle();
                     ack.service = message.service.name;

                     communication::ipc::blocking::send( communication::ipc::broker::device(), ack);
                  }


                  void Default::statistics( platform::ipc::id::type id,  message::traffic::Event& event)
                  {
                     Trace trace{ "server::handle::policy::Default::statistics"};

                     log::debug << "event:" << event << '\n';

                     try
                     {
                        communication::ipc::blocking::send( id, event);
                     }
                     catch( ...)
                     {
                        error::handler();
                     }
                  }

                  namespace local
                  {
                     namespace
                     {
                        template< typename M>
                        void transaction( M& message, const server::Service& service, const platform::time::point::type& now)
                        {
                           Trace trace{ "server::handle::policy::local::transaction"};

                           log::debug << "message: " << message << ", service: " << service << '\n';

                           //
                           // We keep track of callers transaction (can be null-trid).
                           //
                           transaction::Context::instance().caller = message.trid;

                           switch( service.transaction)
                           {
                              case service::transaction::Type::automatic:
                              {
                                 if( message.trid)
                                 {
                                    transaction::Context::instance().join( message.trid);
                                 }
                                 else
                                 {
                                    transaction::Context::instance().start( now);
                                 }
                                 break;
                              }
                              case service::transaction::Type::join:
                              {
                                 transaction::Context::instance().join( message.trid);
                                 break;
                              }
                              case service::transaction::Type::atomic:
                              {
                                 transaction::Context::instance().start( now);
                                 break;
                              }
                              case service::transaction::Type::none:
                              default:
                              {
                                 //
                                 // We don't start or join any transactions
                                 // (technically we join a null-trid)
                                 //
                                 transaction::Context::instance().join( transaction::ID{ process::handle()});
                                 break;
                              }

                           }

                           //
                           // Set 'global deadline'
                           //
                           transaction::Context::instance().current().timout.set( now, message.service.timeout);

                        }

                     } // <unnamed>
                  } // local


                  void Default::transaction( const message::service::call::callee::Request& message, const server::Service& service, const platform::time::point::type& now)
                  {
                     local::transaction( message, service, now);
                  }

                  void Default::transaction( const message::conversation::connect::callee::Request& message, const server::Service& service, const platform::time::point::type& now)
                  {
                     local::transaction( message, service, now);
                  }

                  void Default::transaction( message::service::call::Reply& message, int return_state)
                  {
                     transaction::Context::instance().finalize( message, return_state);
                  }

                  void Default::transaction( message::conversation::caller::Send& message, int return_state)
                  {
                     //transaction::Context::instance().finalize( message, return_state);
                  }

                  namespace local
                  {
                     namespace
                     {
                        template< typename M>
                        void forward( M&& message, const state::Jump& jump)
                        {
                           Trace trace{ "server::handle::policy::local::forward"};


                           if( transaction::Context::instance().pending())
                           {
                              throw common::exception::xatmi::service::Error( "service: " + message.service.name + " tried to forward with pending transactions");
                           }

                           common::service::Lookup lookup{
                              jump.forward.service,
                              message::service::lookup::Request::Context::forward};

                           /*
                           message::service::call::callee::Request request;
                           request.correlation = message.correlation;
                           request.parent = message.service.name;
                           request.descriptor = message.descriptor;
                           request.trid = message.trid;
                           request.process = message.process;
                           request.flags = message.flags;
                           */

                           auto request = message;



                           auto target = lookup();

                           if( target.state == message::service::lookup::Reply::State::absent)
                           {
                              throw common::exception::xatmi::service::no::Entry( target.service.name);
                           }
                           else if( target.state ==  message::service::lookup::Reply::State::busy)
                           {
                              //
                              // We wait for service to become idle
                              //
                              target = lookup();
                           }

                           if( jump.buffer.data == nullptr)
                           {
                              request.buffer = buffer::Payload{ nullptr};
                           }
                           else
                           {
                              request.buffer = buffer::pool::Holder::instance().release( jump.buffer.data, jump.buffer.size);
                           }

                           request.service = target.service;


                           log::debug << "policy::Default::forward - request:" << request << std::endl;

                           communication::ipc::blocking::send( target.process.queue, request);

                        }

                     } // <unnamed>
                  } // local

                  void Default::forward( const message::service::call::callee::Request& message, const state::Jump& jump)
                  {
                     local::forward( message, jump);
                  }


                  void Default::forward( const message::conversation::connect::callee::Request& message, const state::Jump& jump)
                  {
                     local::forward( message, jump);
                  }


                  Admin::Admin( communication::error::type handler)
                     : m_error_handler{ std::move( handler)}
                  {}


                  void Admin::configure( server::Arguments& arguments)
                  {
                     //
                     // Connection to the domain has been done before...
                     //


                     if( ! arguments.resources.empty())
                     {
                        throw common::exception::invalid::Semantic{ "can't build and link an administration server with resources"};
                     }

                     policy::local::configure::services( arguments.services, {});

                  }

                  void Admin::reply( platform::ipc::id::type id, message::service::call::Reply& message)
                  {
                     communication::ipc::blocking::send( id, message, m_error_handler);
                  }

                  void Admin::ack( const message::service::call::callee::Request& message)
                  {
                     message::service::call::ACK ack;
                     ack.process = common::process::handle();
                     ack.service = message.service.name;

                     communication::ipc::blocking::send( communication::ipc::broker::device(), ack, m_error_handler);
                  }


                  void Admin::statistics( platform::ipc::id::type id, message::traffic::Event&)
                  {
                     // no-op
                  }

                  void Admin::transaction( const message::service::call::callee::Request&, const server::Service&, const common::platform::time::point::type&)
                  {
                     // no-op
                  }
                  void Admin::transaction( message::service::call::Reply& message, int return_state)
                  {
                     // no-op
                  }

                  void Admin::forward( const common::message::service::call::callee::Request& message, const state::Jump& jump)
                  {
                     throw common::exception::xatmi::System{ "can't forward within an administration server"};
                  }

               } // call

            } // policy

         } // handle
      } // server
   } // common
} // casual
