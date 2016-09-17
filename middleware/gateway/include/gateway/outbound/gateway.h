//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_GATEWAY_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_GATEWAY_H_

#include "gateway/message.h"
#include "gateway/common.h"
#include "gateway/handle.h"
#include "gateway/environment.h"

#include "gateway/outbound/routing.h"

#include "common/communication/ipc.h"

#include "common/message/dispatch.h"
#include "common/message/gateway.h"
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

            template< typename M, typename D>
            struct basic_forward
            {
               using message_type = M;
               using device_type = D;

               basic_forward( device_type& device)
                  : device( device) {}

               void operator() ( message_type& message)
               {
                  this->device.blocking_send( message);
               }

               device_type& device;
            };

            namespace forward
            {
               template< typename M, typename D>
               auto create( D& device) -> basic_forward< M, D>
               {
                  return basic_forward< M, D>( device);
               }
            } // forward


            template< typename M, typename D>
            struct basic_request
            {
               using message_type = M;
               using device_type = D;


               basic_request( const Routing& routing, device_type& device)
                  : routing( routing), device( device) {}

               void operator() ( message_type& message)
               {
                  routing.add( message.correlation, message.process);

                  auto&& request = message::interdomain::send::wrap( message);
                  device.blocking_send( request);
               }

               const Routing& routing;
               device_type& device;
            };

            template< typename M, typename D>
            basic_request< M, D> create( const Routing& routing, D& device)
            {
               return basic_request< M, D>{ routing, device};
            }

            namespace call
            {

               template< typename D>
               struct Request : basic_request< common::message::service::call::callee::Request, D>
               {
                  using message_type = common::message::service::call::callee::Request;
                  using request_base = basic_request< common::message::service::call::callee::Request, D>;

                  Request( const Routing& routing, D& device, const common::domain::Identity& remote)
                     : request_base{ routing, device}, m_remote( remote) {}

                  void operator() ( message_type& message)
                  {
                     log << "call request: " << message << '\n';

                     if( ! common::flag< TPNOREPLY>( message.flags))
                     {
                        if( message.trid)
                        {
                           //
                           // Notify TM that this "resource" is involved in the transaction
                           //
                           common::communication::ipc::blocking::send(
                                 common::communication::ipc::transaction::manager::device(),
                                 common::message::transaction::resource::domain::involved::create(
                                       m_remote.id, message));
                        }

                        this->routing.add( message.correlation, message.process);
                     }

                     auto&& request = message::interdomain::send::wrap( message);
                     this->device.blocking_send( request);
                  }

               private:
                  const common::domain::Identity& m_remote;
               };

            } // call

            namespace queue
            {
               namespace enqueue
               {

               } // enqueue

               namespace dequeue
               {

               } // dequeue

            } // queue


            template< typename M>
            struct basic_reply : Base
            {
               using message_type = M;
               using Base::Base;

               void operator() ( message_type& message) const
               {
                  auto&& reply = message.get();

                  log << "reply: " << reply << '\n';

                  try
                  {
                     auto destination = routing.get( reply.correlation);
                     process( reply);
                     common::communication::ipc::blocking::send( destination.destination.queue, reply);
                  }
                  catch( const common::exception::queue::Unavailable&)
                  {
                     log << "destination queue unavailable for correlation: " << reply.correlation << " - action: discard\n";
                  }
                  catch( const common::exception::invalid::Argument&)
                  {
                     common::log::error << "failed to correlate ["  << reply.correlation << "] reply with a destination - action: ignore\n";
                  }
               }

            private:
               void process( common::message::service::call::Reply& message) const { }

               template< typename T>
               void process( T& message) const { message.process = common::process::handle();}
            };



            namespace domain
            {
               namespace id
               {
                  struct Message : common::message::basic_message< common::message::Type::gateway_domain_id>
                  {
                     common::domain::Identity id;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & id;
                     })
                  };


               } // id

               struct Identity
               {
                  Identity( common::domain::Identity& id) : id( id) {}

                  void operator () ( id::Message& message)
                  {
                     Trace trace{ "gateway::outbound::handle::domain::Identity"};

                     id = message.id;
                  }

                  common::domain::Identity& id;
               };

               namespace discover
               {
                  struct Reply : basic_reply< message::interdomain::domain::discovery::receive::Reply>
                  {
                     using base_type = basic_reply< message::interdomain::domain::discovery::receive::Reply>;

                     using base_type::base_type;

                     void operator () ( message::interdomain::domain::discovery::receive::Reply& reply) const
                     {
                        Trace trace{ "gateway::outbound::handle::domain::discover::Reply"};

                        //
                        // Send information about remote id to main thread, once
                        //
                        common::execute::once( [&](){

                           Trace trace{ "gateway::outbound::handle::domain::discover::Reply send domain id"};

                           domain::id::Message message;
                           message.id = reply.domain;
                           common::communication::ipc::blocking::send( common::communication::ipc::inbound::id(), message);
                        });

                        base_type::operator() ( reply);
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
            {

               //
               // 'connect' to our local domain
               //
               common::process::instance::connect();

               m_reply_thread = std::thread{ &reply_thread< S>, std::ref( m_routing), std::forward< S>( settings)};
            }



            ~Gateway()
            {
               try
               {
                  Trace trace{ "gateway::outbound::Gateway::~Gateway()"};

                  if( m_reply_thread.joinable())
                  {

                     //
                     // We send user-signal to worker thread, and we make sure we can't get the
                     // signal
                     //
                     common::signal::thread::scope::Block block;

                     common::signal::thread::send( m_reply_thread, common::signal::Type::user);

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
               Trace trace{ "gateway::outbound::Gateway::operator()"};

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

               log << "internal policy: " << policy << '\n';


               //
               // Send connect to gateway so it knows we're up'n running
               //
               {
                  Trace trace{ "gateway::outbound::Gateway::operator() gateway connect"};

                  message::outbound::Connect connect;
                  connect.process = common::process::handle();
                  connect.address = policy.address( outbound_device);

                  common::communication::ipc::blocking::send( common::communication::ipc::gateway::manager::device(), connect);
               }

               common::domain::Identity remote;

               using outbound_device_type = decltype( outbound_device);

               common::message::dispatch::Handler handler{
                  //
                  // internal messages
                  //
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  gateway::handle::Disconnect{ m_reply_thread},
                  handle::domain::Identity{ remote},

                  //
                  // external messages, that will be forward to remote domain
                  //
                  handle::call::Request< outbound_device_type>{ m_routing, outbound_device, remote},
                  handle::create< common::message::transaction::resource::prepare::Request>( m_routing, outbound_device),
                  handle::create< common::message::transaction::resource::commit::Request>( m_routing, outbound_device),
                  handle::create< common::message::transaction::resource::rollback::Request>( m_routing, outbound_device),

                  handle::create< common::message::gateway::domain::discover::internal::Request>( m_routing, outbound_device),



               };

               common::message::dispatch::pump( handler, common::communication::ipc::inbound::device(), ipc_policy{});
            }

         private:

            configuration_type connect()
            {
               Trace trace{ "gateway::outbound::Gateway::connect"};

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

               configuration_type configuration;

               common::marshal::binary::Input marshal{ message.information};
               marshal >> configuration;

               return configuration;
            }

            template< typename S>
            static void reply_thread( const Routing& routing, S&& settings)
            {
               //
               // We're only interested in sig-user
               //
               common::signal::thread::scope::Mask block{ common::signal::set::filled( { common::signal::Type::user})};

               Trace trace{ "gateway::outbound::Gateway::reply_thread"};


               auto send_disconnect = []( message::worker::Disconnect::Reason reason)
                  {
                     common::communication::ipc::outbound::Device ipc{ common::communication::ipc::inbound::id()};

                     message::worker::Disconnect disconnect{ reason};
                     log << "send disconnect: " << disconnect << '\n';

                     ipc.send( disconnect, common::communication::ipc::policy::Blocking{});
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

                     common::Trace trace{ "gateway::outbound::Gateway::reply_thread main thread connect", log};

                     message::worker::Connect message;
                     //message.address = policy.address();

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

                     //
                     // External messages from the other domain
                     //
                     handle::basic_reply< message::interdomain::service::call::receive::Reply>{ routing},
                     handle::basic_reply< message::interdomain::transaction::resource::receive::prepare::Reply>{ routing},
                     handle::basic_reply< message::interdomain::transaction::resource::receive::commit::Reply>{ routing},
                     handle::basic_reply< message::interdomain::transaction::resource::receive::rollback::Reply>{ routing},
                     handle::domain::discover::Reply{ routing},
                  };

                  log << "start reply message pump\n";

                  common::message::dispatch::blocking::pump( handler, inbound);

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

            const Routing m_routing;
            std::thread m_reply_thread;
         };


      } // outbound
   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_GATEWAY_H_
