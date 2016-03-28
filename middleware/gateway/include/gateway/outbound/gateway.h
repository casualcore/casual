//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_GATEWAY_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_GATEWAY_H_

#include "gateway/message.h"
#include "gateway/handle.h"
#include "gateway/environment.h"

#include "gateway/outbound/routing.h"

#include "common/communication/ipc.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/message/service.h"
#include "common/message/transaction.h"

#include "common/flag.h"

#include "common/trace.h"

namespace casual
{
   namespace gateway
   {
      namespace outbound
      {
         namespace handle
         {

            struct Base
            {
               Base( const Routing& routing) : routing( routing) {}

               const Routing& routing;
            };

            template< typename D>
            struct base_request : Base
            {
               using device_type = D;

               base_request( const Routing& routing, device_type& device)
                  : Base( routing), device( device) {}


               device_type& device;
            };

            template< typename M, typename D>
            struct basic_request : base_request< D>
            {
               using message_type = M;
               using base_request< D>::base_request;

               void operator() ( message_type& message)
               {
                  this->device.blocking_send( message);
                  this->routing.add( message.correlation, message.process);
               }
            };

            template< typename M, typename D>
            basic_request< M, D> create( const Routing& routing, D& device)
            {
               return basic_request< M, D>{ routing, device};
            }

            namespace call
            {
               template< typename D>
               struct Request : base_request< D>
               {
                  using message_type = common::message::service::call::callee::Request;
                  using base_request< D>::base_request;

                  void operator() ( message_type& message)
                  {
                     common::log::internal::gateway << "call request: " << message << '\n';

                     if( message.trid)
                     {
                        //
                        // We need to notify TM that this gateway is involved in the
                        // transaction
                        //
                        // TODO!
                     }
                     this->device.blocking_send( message);

                     if( ! common::flag< TPNOREPLY>( message.flags))
                     {
                        this->routing.add( message.correlation, message.process);
                     }
                  }
               };

            } // call


            template< typename M>
            struct basic_reply : Base
            {
               using message_type = M;
               using Base::Base;

               void operator() ( message_type& message)
               {
                  common::log::internal::gateway << "reply: " << message << '\n';

                  try
                  {
                     auto destination = routing.get( message.correlation);
                     common::communication::ipc::blocking::send( destination.destination.queue, message);
                  }
                  catch( const common::exception::queue::Unavailable&)
                  {
                     common::log::internal::gateway << "destination queue: " << routing.get( message.correlation)
                           << " unavailable for correlation: " << message.correlation << " - action: discard\n";
                  }
                  catch( const common::exception::invalid::Argument&)
                  {
                     common::log::error << "failed to correlate ["  << message.correlation << "] reply with a destination - action: ignore\n";
                  }
               }
            };

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
                //: m_reply_thread{ reply_thread< S>, std::ref( m_routing), std::forward< S>( settings)}
            {

               //
               // 'connect' to our local domain
               //
               common::process::connect();

               m_reply_thread = std::thread{ reply_thread< S>, std::ref( m_routing), std::forward< S>( settings)};
            }



            ~Gateway()
            {
               try
               {
                  common::Trace trace{ "gateway::outbound::Gateway::~Gateway()", common::log::internal::gateway};

                  if( m_reply_thread.joinable())
                  {

                     //
                     // We send sig-term to worker thread, and we make sure we can't get the
                     // signal
                     //
                     common::signal::thread::scope::Block block;

                     common::signal::thread::send( m_reply_thread, common::signal::Type::terminate);

                     //
                     // Worker will always send Disconnect, we'll consume it...
                     //
                     {
                        message::worker::Disconnect disconnect;
                        common::communication::ipc::inbound::device().receive( disconnect, ipc_policy{});
                     }


                     m_reply_thread.join();
                  }

               }
               catch( ...)
               {
                  common::error::handler();
               }

            }

            void operator() ()
            {
               common::Trace trace{ "gateway::outbound::Gateway::operator()", common::log::internal::gateway};

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

               common::log::internal::gateway << "internal policy: " << policy << '\n';


               //
               // Send connect to gateway so it knows we're up'n running
               //
               {
                  common::Trace trace{ "gateway::outbound::Gateway::operator() gateway connect", common::log::internal::gateway};

                  message::outbound::Connect connect;
                  connect.process = common::process::handle();
                  connect.remote = m_remote;

                  environment::manager::device().send( connect, ipc_policy{});
               }

               using outbound_device_type = decltype( outbound_device);

               common::message::dispatch::Handler handler{
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  gateway::handle::Disconnect{ m_reply_thread},
                  handle::call::Request< outbound_device_type>{ m_routing, outbound_device},
                  handle::create< common::message::transaction::resource::domain::prepare::Request>( m_routing, outbound_device),
                  handle::create< common::message::transaction::resource::domain::commit::Request>( m_routing, outbound_device),
                  handle::create< common::message::transaction::resource::domain::rollback::Request>( m_routing, outbound_device),
               };

               common::message::dispatch::pump( handler, common::communication::ipc::inbound::device(), ipc_policy{});
            }

         private:

            configuration_type connect()
            {
               common::Trace trace{ "gateway::outbound::Gateway::connect", common::log::internal::gateway};

               message::worker::Connect message;

               common::message::dispatch::Handler handler{
                  gateway::handle::Disconnect{ m_reply_thread},
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
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
            static void reply_thread( const Routing& routing, S&& settings)
            {
               common::Trace trace{ "gateway::outbound::Gateway::reply_thread", common::log::internal::gateway};


               auto send_disconnect = []( message::worker::Disconnect::Reason reason)
                  {
                     common::communication::ipc::blocking::send(
                        common::communication::ipc::inbound::id(),
                        message::worker::Disconnect{ reason});
                  };

               try
               {
                  //
                  // Instantiate the external policy
                  //
                  external_policy_type policy{ std::forward< S>( settings)};


                  auto&& inbound = policy.inbound();


                  //
                  // Send connection to the main thread so it knows how to communicate with the other
                  // domain
                  //
                  {

                     common::Trace trace{ "gateway::outbound::Gateway::reply_thread main thread connect", common::log::internal::gateway};

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
                  // we start our reply-message-pump
                  //
                  common::message::dispatch::Handler handler{
                     handle::basic_reply< common::message::service::call::Reply>{ routing},
                     handle::basic_reply< common::message::transaction::resource::domain::prepare::Reply>{ routing},
                     handle::basic_reply< common::message::transaction::resource::domain::commit::Reply>{ routing},
                     handle::basic_reply< common::message::transaction::resource::domain::rollback::Reply>{ routing},
                  };

                  common::log::internal::gateway << "start reply message pump\n";

                  common::message::dispatch::blocking::pump( handler, inbound);

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

            Routing m_routing;
            std::thread m_reply_thread;
            common::domain::Identity m_remote;
         };


      } // outbound
   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_GATEWAY_H_
