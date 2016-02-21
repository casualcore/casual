//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_GATEWAY_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_GATEWAY_H_

#include "gateway/inbound/cache.h"

#include "gateway/message.h"


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
            void send( common::platform::queue_id_type id, M&& message)
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
                        auto request = m_cache.get( message.correlation);

                        common::communication::ipc::outbound::Device ipc{ message.process.queue};
                        ipc.put( request, common::communication::ipc::policy::ignore::signal::Blocking{});
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
                  m_device.blocking_send( message);
               }

            private:
               device_type& m_device;
            };


            struct Disconnect
            {
               using message_type = message::worker::Disconnect;

               void operator() ( message_type& message)
               {
                  // TODO: we may need to distinguish disconnect from shutdown...
                  throw common::exception::Shutdown{ "disconnected"};
               }

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
            using outbound_configuration = typename policy_type::outbound_configuration;

            template< typename S>
            Gateway( S&& settings)
              : m_request_thread{ request_thread< S>, std::ref( m_cache), validate_settings( std::forward< S>( settings))}
            {
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

                     m_request_thread.join();
                  }

               }
               catch( ...)
               {
                  common::error::handler();
               }
            }

            template< typename S>
            static auto validate_settings( S&& settings)
               -> decltype( std::forward< S>( settings))
            {
               policy_type::validate( settings);
               return std::forward< S>( settings);
            }

            void operator() ()
            {
               common::Trace trace{ "gateway::inbound::Gateway::operator()", common::log::internal::gateway};

               //
               // Now we wait for the worker to establish connection with
               // the other domain. We're still active and can be shut down
               //

               auto outbound_device = policy_type::outbound_device( connect());


               common::message::dispatch::Handler handler{
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  handle::Disconnect{},
                  handle::create::reply< common::message::service::call::Reply>( outbound_device),
                  handle::create::reply< common::message::transaction::resource::domain::prepare::Request>( outbound_device),
                  handle::create::reply< common::message::transaction::resource::domain::commit::Request>( outbound_device),
                  handle::create::reply< common::message::transaction::resource::domain::rollback::Request>( outbound_device),
               };

               common::log::internal::gateway << "start message pump\n";
               common::message::dispatch::blocking::pump( handler, common::communication::ipc::inbound::device());

            }

         private:

            static outbound_configuration connect()
            {
               common::Trace trace{ "gateway::inbound::Gateway::connect", common::log::internal::gateway};

               common::communication::ipc::Helper ipc;

               message::worker::Connect message;

               common::message::dispatch::Handler handler{
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  common::message::handle::assign( message),
               };

               while( ! message.execution)
               {
                  handler( ipc.blocking_next());
               }

               outbound_configuration configuration;
               common::marshal::binary::Input marshal{ message.information};
               marshal >> configuration;

               return configuration;
            }

            template< typename S>
            static void request_thread( const Cache& cache, S&& settings)
            {
               common::Trace trace{ "gateway::inbound::Gateway::request_thread", common::log::internal::gateway};

               try
               {
                  //
                  // Keep a state just in case this device need one...
                  //
                  auto state = policy_type::worker_state( std::forward< S>( settings));

                  auto&& device = policy_type::connect( state);


                  //
                  // Send connection to the main thread so it knows how to communicate with the other
                  // domain
                  //
                  {
                     message::worker::Connect message;

                     {
                        auto configuration = policy_type::outbound_device( state);
                        common::marshal::binary::Output marshal{ message.information};
                        marshal << configuration;
                     }

                     common::communication::ipc::blocking::send( common::communication::ipc::inbound::id(), message);
                  }

                  common::log::information << "connection established - state: " << state << "\n";

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
               catch( const common::exception::Shutdown&)
               {
                  // no op
               }
               catch( ...)
               {
                  common::error::handler();
               }

            }

            Cache m_cache;
            std::thread m_request_thread;
         };

      } // inbound
   } // gateway



} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_GATEWAY_H_
