//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_GATEWAY_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_GATEWAY_H_

#include "gateway/inbound/cache.h"

#include "gateway/message.h"
#include "gateway/handle.h"
#include "gateway/environment.h"


#include "common/marshal/complete.h"
#include "common/communication/ipc.h"

#include "common/message/service.h"
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
            template< typename M>
            void send( common::platform::ipc::id::type id, M&& message)
            {
               common::communication::ipc::outbound::Device ipc{ id};
               ipc.send( std::forward< M>( message), common::communication::ipc::policy::ignore::signal::Blocking{});
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
                  blocking::send( common::process::instance::transaction::manager::handle().queue, message);
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
                     common::Trace trace{ "gateway::inbound::handle::call::Request::operator()", common::log::internal::gateway};

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
                     blocking::send( common::communication::ipc::broker::id(), request);
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
                        common::log::internal::gateway << "lookup reply: " << message << "\n";

                        auto request = m_cache.get( message.correlation);

                        switch( message.state)
                        {
                           case message_type::State::idle:
                           {
                              try
                              {
                                 common::communication::ipc::outbound::Device ipc{ message.process.queue};
                                 ipc.put( request, common::communication::ipc::policy::ignore::signal::Blocking{});
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
            struct basic_reply
            {
               using message_type = M;
               using device_type = D;

               basic_reply( device_type& device) : m_device( device) {}

               void operator() ( message_type& message)
               {
                  common::Trace trace{ "gateway::inbound::handle::basic_reply::operator()", common::log::internal::gateway};

                  m_device.blocking_send( message);
               }

            private:
               device_type& m_device;
            };


            namespace create
            {
               template< typename M, typename D>
               auto reply( D& device) -> basic_reply< M, D>
               {
                  return basic_reply< M, D>{ device};
               }

            } // create



         } // handle

         template< typename Policy>
         struct Gateway
         {
            using policy_type = Policy;
            using configuration_type = typename policy_type::configuration_type;
            using internal_policy_type = typename policy_type::internal_type;
            using external_policy_type = typename policy_type::external_type;

            using ipc_policy = common::communication::ipc::policy::ignore::signal::Blocking;

            template< typename S>
            Gateway( S&& settings)
              : m_request_thread{ request_thread< S>, std::ref( m_cache), std::forward< S>( settings)}
            {
               //
               // 'connect' to our local domain
               //
               common::process::connect();
            }

            ~Gateway()
            {
               try
               {
                  common::Trace trace{ "gateway::inbound::Gateway::~Gateway()", common::log::internal::gateway};

                  if( m_request_thread.joinable())
                  {

                     //
                     // We send sig-term to worker thread, and we make sure we can't get the
                     // signal
                     //
                     common::signal::thread::scope::Block block;

                     common::signal::thread::send( m_request_thread, common::signal::Type::terminate);

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
               common::Trace trace{ "gateway::inbound::Gateway::operator()", common::log::internal::gateway};

               //
               // We block all signals, so worker thread gets all of'em
               //
               common::signal::thread::scope::Block block;

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
                  common::Trace trace{ "gateway::inbound::Gateway::operator() gateway connect", common::log::internal::gateway};

                  message::inbound::Connect connect;
                  connect.process = common::process::handle();
                  connect.remote = m_remote;
                  environment::manager::device().send( connect, ipc_policy{});
               }


               common::message::dispatch::Handler handler{
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  gateway::handle::Disconnect{ m_request_thread},
                  handle::call::lookup::Reply{ m_cache},
                  handle::create::reply< common::message::service::call::Reply>( outbound_device),
                  handle::create::reply< common::message::transaction::resource::domain::prepare::Request>( outbound_device),
                  handle::create::reply< common::message::transaction::resource::domain::commit::Request>( outbound_device),
                  handle::create::reply< common::message::transaction::resource::domain::rollback::Request>( outbound_device),
               };

               common::log::internal::gateway << "start message pump\n";
               common::message::dispatch::pump( handler, common::communication::ipc::inbound::device(), ipc_policy{});

            }

         private:

            configuration_type connect()
            {
               common::Trace trace{ "gateway::inbound::Gateway::connect", common::log::internal::gateway};

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

               m_remote = message.remote;

               configuration_type configuration;
               common::marshal::binary::Input marshal{ message.information};
               marshal >> configuration;

               return configuration;
            }

            template< typename S>
            static void request_thread( const Cache& cache, S&& settings)
            {
               common::Trace trace{ "gateway::inbound::Gateway::request_thread", common::log::internal::gateway};

               auto send_disconnect = []( message::worker::Disconnect::Reason reason)
                  {
                     common::communication::ipc::blocking::send(
                        common::communication::ipc::inbound::id(),
                        message::worker::Disconnect{ reason});
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
                     common::Trace trace{ "gateway::inbound::Gateway::request_thread main thread connect", common::log::internal::gateway};

                     message::worker::Connect message;

                     message.remote = policy.remote();

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
                  };

                  common::message::dispatch::blocking::pump( handler, device);
               }
               catch( const common::exception::signal::Terminate&)
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
            common::domain::Identity m_remote;
         };

      } // inbound
   } // gateway



} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_GATEWAY_H_
