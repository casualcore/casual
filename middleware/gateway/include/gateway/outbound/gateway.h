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
            struct State
            {
               const Routing routing;
               common::domain::Identity remote;
            };


            struct Base
            {
               Base( const State& state) : state( state) {}

               const State& state;
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


               basic_request( const State& state, device_type& device)
                  : state( state), forward( device) {}

               void operator() ( message_type& message)
               {
                  forward( message);
                  state.routing.add( message.correlation, message.process);
               }

               const State& state;
               basic_forward< message_type, device_type> forward;
            };

            template< typename M, typename D>
            basic_request< M, D> create( const State& state, D& device)
            {
               return basic_request< M, D>{ state, device};
            }

            namespace call
            {
               template< typename D>
               struct Request : basic_request< common::message::service::call::callee::Request, D>
               {
                  using message_type = common::message::service::call::callee::Request;
                  using basic_request< common::message::service::call::callee::Request, D>::basic_request;

                  void operator() ( message_type& message)
                  {
                     log << "call request: " << message << '\n';


                     this->forward( message);

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
                                    this->state.remote.id, message));
                        }

                        this->state.routing.add( message.correlation, message.process);
                     }
                  }
               };

            } // call


            template< typename M>
            struct basic_reply : Base
            {
               using message_type = M;
               using Base::Base;

               void operator() ( message_type& message) const
               {
                  log << "reply: " << message << '\n';

                  try
                  {
                     auto destination = state.routing.get( message.correlation);
                     process( message);
                     common::communication::ipc::blocking::send( destination.destination.queue, message);
                  }
                  catch( const common::exception::queue::Unavailable&)
                  {
                     log << "destination queue unavailable for correlation: " << message.correlation << " - action: discard\n";
                  }
                  catch( const common::exception::invalid::Argument&)
                  {
                     common::log::error << "failed to correlate ["  << message.correlation << "] reply with a destination - action: ignore\n";
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
                  struct Request
                  {
                     Request( std::vector< std::string> address) : address( std::move( address)) {}

                     void operator () ( common::message::gateway::domain::discover::Request& request) const
                     {
                        Trace trace{ "gateway::outbound::handle::domain::discover::Request"};

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

                  using reply_base = basic_reply< common::message::gateway::domain::discover::Reply>;
                  struct Reply : reply_base
                  {
                     using reply_base::reply_base;

                     void operator () ( common::message::gateway::domain::discover::Reply& reply) const
                     {
                        //
                        // Send information about remote id to main thread
                        //
                        {
                           domain::id::Message message;
                           message.id = reply.remote;
                           common::communication::ipc::blocking::send( common::communication::ipc::inbound::id(), message);
                        }
                        reply_base::operator() ( reply);

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

               m_reply_thread = std::thread{ &reply_thread< S>, std::ref( m_state), std::forward< S>( settings)};
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

                  common::communication::ipc::blocking::send( common::communication::ipc::gateway::manager::device(), connect);
               }

               using outbound_device_type = decltype( outbound_device);

               common::message::dispatch::Handler handler{
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  gateway::handle::Disconnect{ m_reply_thread},
                  handle::call::Request< outbound_device_type>{ m_state, outbound_device},
                  handle::create< common::message::transaction::resource::domain::prepare::Request>( m_state, outbound_device),
                  handle::create< common::message::transaction::resource::domain::commit::Request>( m_state, outbound_device),
                  handle::create< common::message::transaction::resource::domain::rollback::Request>( m_state, outbound_device),
                  handle::create< common::message::gateway::domain::discover::Request>( m_state, outbound_device),

                  //
                  // This is a message from worker thread to reply to an information request from other domain
                  //
                  handle::forward::create< common::message::gateway::domain::discover::Reply>( outbound_device),

                  handle::domain::Identity{ m_state.remote},

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
            static void reply_thread( const handle::State& state, S&& settings)
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
                     handle::basic_reply< common::message::service::call::Reply>{ state},
                     handle::basic_reply< common::message::transaction::resource::domain::prepare::Reply>{ state},
                     handle::basic_reply< common::message::transaction::resource::domain::commit::Reply>{ state},
                     handle::basic_reply< common::message::transaction::resource::domain::rollback::Reply>{ state},
                     handle::domain::discover::Reply{ state},

                     handle::domain::discover::Request{ policy.address()},
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

            handle::State m_state;
            std::thread m_reply_thread;
         };


      } // outbound
   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_GATEWAY_H_
