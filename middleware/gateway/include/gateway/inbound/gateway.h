//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_GATEWAY_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_GATEWAY_H_

#include "gateway/inbound/cache.h"

#include "gateway/message.h"
#include "gateway/common.h"
#include "gateway/handle.h"
#include "gateway/environment.h"


#include "common/marshal/complete.h"
#include "common/communication/ipc.h"

#include "common/message/service.h"
#include "common/message/gateway.h"
#include "common/message/transaction.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"

#include "common/trace.h"

namespace casual
{
   namespace gateway
   {
      namespace inbound
      {
         namespace blocking
         {
            template< typename D, typename M>
            void send( D&& device, M&& message)
            {
               common::signal::thread::scope::Block block;
               common::communication::ipc::blocking::send( std::forward< D>( device), std::forward< M>( message));
            }

         } // blocking

         namespace handle
         {




            template< typename M>
            struct basic_transaction_request
            {
               using message_type = M;

               void operator() ( message_type& message)
               {
                  //
                  // Change 'sender' so we (our main thread) get the reply
                  //
                  message.process = common::process::handle();
                  blocking::send( common::communication::ipc::transaction::manager::device(), message);
               }
            };



            namespace call
            {
               struct Base
               {
                  Base( const Cache& cache) : m_cache( cache) {}

               protected:
                  const Cache& m_cache;
               };

               struct Request : Base
               {
                  using message_type = common::message::service::call::callee::Request;

                  using Base::Base;


                  void operator() ( message_type& message)
                  {
                     Trace trace{ "gateway::inbound::handle::call::Request::operator()"};

                     //
                     // Change 'sender' so we (our main thread) get the reply
                     //
                     message.process = common::process::handle();

                     //
                     // Prepare lookup
                     //
                     common::message::service::lookup::Request request;
                     {
                        request.correlation = message.correlation;
                        request.requested = message.service.name;
                        request.context = common::message::service::lookup::Request::Context::gateway;
                        request.process = common::process::handle();
                     }

                     //
                     // Add message to cache, this could block
                     //
                     m_cache.add( common::marshal::complete( std::move( message)));

                     //
                     // Send lookup
                     //
                     blocking::send( common::communication::ipc::broker::device(), request);
                  }
               };

               namespace lookup
               {
                  struct Reply : Base
                  {
                     using message_type = common::message::service::lookup::Reply;

                     using Base::Base;

                     void operator() ( message_type& message)
                     {
                        log << "lookup reply: " << message << "\n";

                        auto request = m_cache.get( message.correlation);

                        switch( message.state)
                        {
                           case message_type::State::idle:
                           {
                              try
                              {
                                 common::communication::ipc::outbound::Device ipc{ message.process.queue};
                                 ipc.put( request, common::communication::ipc::policy::Blocking{});
                              }
                              catch( const common::exception::queue::Unavailable&)
                              {
                                 common::log::error << "server: " << message.process << " has been terminated during interdomain call - action: reply with TPESVCERR\n";
                                 send_error_reply( message);
                              }
                              break;
                           }
                           case message_type::State::absent:
                           {
                              common::log::error << "service: " << message.service << " is not handled by this domain (any more) - action: reply with TPESVCERR\n";
                              send_error_reply( message);
                              break;
                           }
                           default:
                           {
                              common::log::error << "unexpected state on lookup reply: " << message << " - action: drop message\n";
                              break;
                           }
                        }
                     }

                  private:
                     void send_error_reply( message_type& message)
                     {
                        common::message::service::call::Reply reply;
                        reply.correlation = message.correlation;
                        reply.error = TPESVCERR;
                        common::communication::ipc::inbound::device().push( reply);
                     }
                  };

               } // lookup

            } // call


            template< typename M, typename D>
            struct basic_forward
            {
               using message_type = M;
               using device_type = D;

               basic_forward( device_type& device) : m_device( device) {}

               void operator() ( message_type& message)
               {
                  Trace trace{ "gateway::inbound::handle::basic_reply::operator()"};

                  m_device.blocking_send( message);
               }

            private:
               device_type& m_device;
            };


            namespace create
            {
               template< typename M, typename D>
               auto forward( D& device) -> basic_forward< M, D>
               {
                  return basic_forward< M, D>{ device};
               }

            } // create

            namespace domain
            {
               namespace discover
               {
                  struct Request
                  {
                     Request( std::vector< std::string> address) : address( std::move( address)) {}

                     void operator () ( common::message::gateway::domain::discover::Request& request) const
                     {
                        Trace trace{ "gateway::inbound::handle::connection::information::Request"};

                        auto reply = common::message::reverse::type( request);

                        reply.process = common::process::handle();
                        reply.remote = common::domain::identity();
                        reply.address = address;

                        //
                        // Send to main thread, for forward to remote domain
                        //
                        common::communication::ipc::blocking::send( common::communication::ipc::inbound::id(), reply);

                     }

                     std::vector< std::string> address;
                  };

                  //!
                  //! Assumes it's the gateway-manager that has sent the request
                  //!
                  //! @todo: make this more generic?
                  //!
                  struct Reply
                  {
                     void operator () ( common::message::gateway::domain::discover::Reply& reply) const
                     {
                        Trace trace{ "gateway::inbound::handle::connection::information::Reply"};

                        reply.process = common::process::handle();
                        common::communication::ipc::blocking::send( common::communication::ipc::gateway::manager::device(), reply);
                     }
                  };


               } // discover

            } // domain


         } // handle

         template< typename Policy>
         struct Gateway
         {
            using policy_type = Policy;
            using configuration_type = typename policy_type::configuration_type;
            using internal_policy_type = typename policy_type::internal_type;
            using external_policy_type = typename policy_type::external_type;

            using ipc_policy = common::communication::ipc::policy::Blocking;

            template< typename S>
            Gateway( S&& settings)
              : m_request_thread{ request_thread< S>, std::ref( m_cache), std::forward< S>( settings)}
            {
               //
               // 'connect' to our local domain
               //
               common::process::instance::connect();

               //
               // We need to make sure that we got the broker queue.
               // the worker thread is the one that sends request to broker, hence
               // is the first one to initialize broker queue lookup, and
               // it's possible that the main thread consumes the process-lookup-reply
               //
               // TOOD: Can we do something about this?
               //
               common::communication::ipc::broker::device();

            }

            ~Gateway()
            {
               try
               {
                  Trace trace{ "gateway::inbound::Gateway::~Gateway()"};

                  if( m_request_thread.joinable())
                  {

                     //
                     // We send sig-term to worker thread, and we make sure we can't get the
                     // signal
                     //
                     common::signal::thread::scope::Block block;

                     common::signal::thread::send( m_request_thread, common::signal::Type::user);

                     //
                     // Worker will always send Disconnect, we'll consume it...
                     //
                     {
                        message::worker::Disconnect disconnect;
                        common::communication::ipc::inbound::device().receive( disconnect, ipc_policy{});
                     }

                     m_request_thread.join();
                  }

               }
               catch( ...)
               {
                  common::error::handler();
               }
            }


            void operator() ()
            {
               Trace trace{ "gateway::inbound::Gateway::operator()"};

               //
               // We block sig-user so worker always gets'em
               //
               common::signal::thread::scope::Block block{ { common::signal::Type::user}};

               //
               // Now we wait for the worker to establish connection with
               // the other domain. We're still active and can be shut down
               //
               internal_policy_type policy{ connect()};


               auto&& outbound_device = policy.outbound();

               //
               // Send connect to gateway so it knows we're up'n running
               //
               {
                  Trace trace{ "gateway::inbound::Gateway::operator() gateway connect"};

                  message::inbound::Connect connect;
                  connect.process = common::process::handle();
                  common::communication::ipc::blocking::send( common::communication::ipc::gateway::manager::device(), connect);
               }


               common::message::dispatch::Handler handler{
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  gateway::handle::Disconnect{ m_request_thread},
                  handle::call::lookup::Reply{ m_cache},
                  handle::create::forward< common::message::service::call::Reply>( outbound_device),
                  handle::create::forward< common::message::transaction::resource::domain::prepare::Reply>( outbound_device),
                  handle::create::forward< common::message::transaction::resource::domain::commit::Reply>( outbound_device),
                  handle::create::forward< common::message::transaction::resource::domain::rollback::Reply>( outbound_device),

                  //
                  // Sent from this domain to get information about the other
                  //
                  handle::create::forward< common::message::gateway::domain::discover::Request>( outbound_device),

                  //
                  // Sent from worker thread to reply to a connection information request
                  //
                  handle::create::forward< common::message::gateway::domain::discover::Reply>( outbound_device),
               };

               log << "start internal message pump\n";
               common::message::dispatch::pump( handler, common::communication::ipc::inbound::device(), ipc_policy{});

            }

         private:

            configuration_type connect()
            {
               Trace trace{ "gateway::inbound::Gateway::connect"};

               message::worker::Connect message;

               common::message::dispatch::Handler handler{
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  gateway::handle::Disconnect{ m_request_thread},
                  common::message::handle::assign( message),
               };

               while( ! message.execution)
               {
                  handler( common::communication::ipc::inbound::device().next( handler.types(), ipc_policy{}));
               }

               configuration_type configuration;
               common::marshal::binary::Input marshal{ message.information};
               marshal >> configuration;

               return configuration;
            }

            template< typename S>
            static void request_thread( const Cache& cache, S&& settings)
            {
               //
               // We're only interested in sig-user
               //
               common::signal::thread::scope::Mask block{ common::signal::set::filled( { common::signal::Type::user})};


               Trace trace{ "gateway::inbound::Gateway::request_thread"};

               auto send_disconnect = []( message::worker::Disconnect::Reason reason)
                  {
                     try
                     {
                        common::signal::thread::scope::Block block;
                        common::communication::ipc::outbound::Device ipc{ common::communication::ipc::inbound::id()};

                        message::worker::Disconnect disconnect{ reason};
                        log << "send disconnect: " << disconnect << '\n';

                        ipc.send( disconnect, common::communication::ipc::policy::Blocking{});
                     }
                     catch( ...)
                     {
                        common::error::handler();
                     }
                  };

               try
               {

                  //
                  // Instantiate external policy, this will connect to remote domain
                  //
                  external_policy_type policy{ std::forward< S>( settings)};

                  auto&& device = policy.device();


                  //
                  // Send connection to the main thread so it knows how to communicate with the other
                  // domain
                  //
                  {
                     Trace trace{ "gateway::inbound::Gateway::request_thread main thread connect"};

                     message::worker::Connect message;

                     {
                        auto configuration = policy.configuration();
                        common::marshal::binary::Output marshal{ message.information};
                        marshal << configuration;
                     }

                     common::communication::ipc::blocking::send( common::communication::ipc::inbound::id(), message);
                  }

                  common::log::information << "connection established - policy: " << policy << "\n";

                  //
                  // we start our request-message-pump
                  //
                  common::message::dispatch::Handler handler{
                     handle::call::Request{ cache},
                     handle::basic_transaction_request< common::message::transaction::resource::domain::prepare::Request>{},
                     handle::basic_transaction_request< common::message::transaction::resource::domain::commit::Request>{},
                     handle::basic_transaction_request< common::message::transaction::resource::domain::rollback::Request>{},

                     handle::domain::discover::Request{ policy.address()},

                     //
                     // Assumes that the reply should be sent to domain-manager
                     //
                     handle::domain::discover::Reply{},

                  };
             
                  log << "start external message pump\n";
                  common::message::dispatch::blocking::pump( handler, device);
               }
               catch( const common::exception::signal::User&)
               {
                  send_disconnect( message::worker::Disconnect::Reason::signal);
               }
               catch( ...)
               {
                  common::error::handler();
                  send_disconnect( message::worker::Disconnect::Reason::disconnect);
               }

            }

            Cache m_cache;
            std::thread m_request_thread;
         };

      } // inbound
   } // gateway



} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_GATEWAY_H_
