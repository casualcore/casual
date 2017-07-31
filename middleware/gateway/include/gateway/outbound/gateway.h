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
#include "common/message/conversation.h"

#include "common/flag.h"


namespace casual
{
   namespace gateway
   {
      namespace outbound
      {
         using size_type = common::platform::size::type;
         
         namespace ipc
         {
            namespace optional
            {
               template< typename D, typename M>
               bool send( D&& destination, M&& message)
               {
                  try
                  {
                     common::communication::ipc::blocking::send( destination, message);
                     return true;
                  }
                  catch( const common::exception::queue::Unavailable&)
                  {
                     log << "destination queue unavailable for correlation: " << message.correlation << " - action: discard\n";
                  }
                  return false;
               }
            } // optional


         } // ipc

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
                  routing.add( message);

                  //auto&& request = message::interdomain::send::wrap( message);

                  // todo: temp
                  if( log)
                  {
                     log << "basic_request::operator() - message: " << message << '\n';
                  }

                  device.blocking_send( message);
               }

               const Routing& routing;
               device_type& device;
            };

            template< typename M, typename D>
            basic_request< M, D> create( const Routing& routing, D& device)
            {
               return basic_request< M, D>{ routing, device};
            }

            namespace service
            {
               namespace call
               {
                  struct Base
                  {
                     Base( const outbound::service::Routing& routing)
                      : routing( routing) {}

                     const outbound::service::Routing& routing;
                  };

                  template< typename D>
                  struct Request
                  {
                     using device_type = D;

                     Request( const outbound::service::Routing& routing, device_type& device)
                     : routing( routing), device( device) {}

                     void operator() ( common::message::service::call::callee::Request& message)
                     {
                        log << "call request: " << message << '\n';

                        auto now = common::platform::time::clock::type::now();

                        if( ! message.flags.exist( common::message::service::call::request::Flag::no_reply))
                        {
                           if( message.trid)
                           {
                              //
                              // Notify TM that this "resource" is involved in the transaction
                              //
                              common::communication::ipc::blocking::send(
                                    common::communication::ipc::transaction::manager::device(),
                                    common::message::transaction::resource::external::involved::create( message));
                           }

                           routing.emplace(
                              message.correlation,
                              message.process,
                              message.service.name,
                              now
                           );
                        }

                        device.blocking_send( message);
                     }

                  private:
                     const outbound::service::Routing& routing;
                     device_type& device;
                  };

               } // call

               namespace conversation
               {
                  namespace connect
                  {
                     template< typename D>
                     struct Request : basic_request< common::message::conversation::connect::callee::Request, D>
                     {
                        using message_type = common::message::conversation::connect::callee::Request;
                        using request_base = basic_request< common::message::conversation::connect::callee::Request, D>;

                        Request( const Routing& routing, D& device)
                        : request_base{ routing, device} {}

                        void operator() ( message_type& message)
                        {
                           log << "conversation connect request: " << message << '\n';

                           if( message.trid)
                           {
                              //
                              // Notify TM that this "resource" is involved in the transaction
                              //
                              common::communication::ipc::blocking::send(
                                    common::communication::ipc::transaction::manager::device(),
                                    common::message::transaction::resource::external::involved::create( message));
                           }

                           this->routing.add( message);

                           this->device.blocking_send( message);
                        }
                     };

                  } // connect

               } // conversation
            } // service


            namespace queue
            {
               template< typename M, typename D>
               struct basic_request : handle::basic_request< M, D>
               {
                  using message_type = M;
                  using request_base = handle::basic_request< M, D>;

                  basic_request( const Routing& routing, D& device)
                     : request_base{ routing, device} {}


                  void operator() ( message_type& message)
                  {
                     Trace trace{ "gateway::outbound::handle::queue::basic_request"};

                     log << "message: " << message << '\n';

                     if( message.trid)
                     {
                        //
                        // Notify TM that this "resource" is involved in the transaction
                        //
                        common::communication::ipc::blocking::send(
                              common::communication::ipc::transaction::manager::device(),
                              common::message::transaction::resource::external::involved::create( message));
                     }

                     this->routing.add( message);

                     this->device.blocking_send( message);
                  }
               };

               namespace enqueue
               {
                  template< typename D>
                  using Request = basic_request< common::message::queue::enqueue::Request, D>;
               } // enqueue

               namespace dequeue
               {
                  template< typename D>
                  using Request = basic_request< common::message::queue::dequeue::Request, D>;
               } // dequeue

            } // queue


            template< typename M>
            struct basic_reply : Base
            {
               using message_type = M;
               using Base::Base;

               void operator() ( message_type& reply) const
               {
                  log << "reply: " << reply << '\n';

                  try
                  {

                     auto destination = routing.get( reply.correlation);
                     process( reply);

                     ipc::optional::send( destination.destination.queue, reply);
                  }
                  catch( const common::exception::invalid::Argument&)
                  {
                     common::log::category::error << "failed to correlate ["  << reply.correlation << "] reply with a destination - action: ignore\n";
                  }
               }

            private:
               void process( common::message::queue::enqueue::Reply& message) const { }
               void process( common::message::queue::dequeue::Reply& message) const { }

               template< typename T>
               void process( T& message) const { message.process = common::process::handle();}
            };



            namespace domain
            {
               namespace discover
               {
                  using base_type = basic_reply< common::message::gateway::domain::discover::Reply>;
                  struct Reply : base_type
                  {
                     Reply( const Routing& routing, size_type order) : base_type{ routing}, m_order{ order} {}

                     void operator () ( common::message::gateway::domain::discover::Reply& reply) const
                     {
                        Trace trace{ "gateway::outbound::handle::domain::discover::Reply"};


                        //
                        // Advertise services and queues.
                        //
                        {
                           Trace trace{ "gateway::outbound::handle::domain::discover::Reply Advertise"};

                           common::message::gateway::domain::Advertise advertise;
                           advertise.execution = reply.execution;
                           advertise.domain = reply.domain;
                           advertise.process = common::process::handle();
                           advertise.order = m_order;
                           advertise.services = reply.services;
                           advertise.queues = reply.queues;

                           if( ! advertise.services.empty())
                           {
                              //
                              // add one hop, since we now it has passed a domain boundary
                              //
                              for( auto& service : advertise.services) { ++service.hops;}

                              ipc::optional::send( common::communication::ipc::service::manager::device(), advertise);
                           }

                           if( ! advertise.queues.empty())
                           {
                              try
                              {
                                 common::communication::ipc::blocking::send(
                                       common::communication::ipc::queue::manager::optional::device(),
                                       advertise);
                              }
                              catch( const common::exception::communication::Unavailable&)
                              {
                                 common::log::category::error << "failed to advertise queues to queue-broker: " << common::range::make( advertise.queues) << '\n';
                              }
                           }
                        }


                        base_type::operator() ( reply);
                     }
                  private:
                     size_type m_order;

                  };


               } // discover
            } // domain

            namespace service
            {
               namespace call
               {
                  struct Reply
                  {
                     Reply( const outbound::service::Routing& routing, common::message::service::remote::Metric& metric)
                        : routing( routing), metric( metric) {}

                     void operator() ( common::message::service::call::Reply& reply) const
                     {
                        log << "reply: " << reply << '\n';

                        try
                        {
                           auto destination = routing.get( reply.correlation);

                           auto now = common::platform::time::clock::type::now();

                           metric.services.emplace_back(
                                 std::move( destination.service),
                                 std::chrono::duration_cast< std::chrono::microseconds>( now - destination.start));

                           ipc::optional::send( destination.destination.queue, reply);
                        }
                        catch( const common::exception::invalid::Argument&)
                        {
                           common::log::category::error << "failed to correlate ["  << reply.correlation << "] reply with a destination - action: ignore\n";
                        }
                     }
                  private:
                     const outbound::service::Routing& routing;
                     common::message::service::remote::Metric& metric;
                  };

               } // call

            } // service

         } // handle

         namespace error
         {
            //!
            //! Takes care of sending error replies to the
            //! request that are in flight when a shutdown is
            //! requested (when connections is lost and such)
            //!
            struct Reply
            {
               Reply( const Routing& routing);
               ~Reply();

            private:
               const Routing& m_routing;
            };
         } // error

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

               m_reply_thread = std::thread{ &reply_thread< S>,
                  std::ref( m_service_routing), std::ref( m_routing), std::forward< S>( settings)};
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

                  //
                  // Make sure we take care of messages in flight
                  //
                  error::Reply{ m_routing};

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
               // Make the initializing dance with remote domain
               //
               initialize( outbound_device, policy.address( outbound_device));


               using outbound_device_type = decltype( outbound_device);

               auto handler = common::communication::ipc::inbound::device().handler(

                  //
                  // internal messages
                  //
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  gateway::handle::Disconnect{ m_reply_thread},

                  //
                  // external messages, that will be forward to remote domain
                  //
                  handle::service::call::Request< outbound_device_type>{ m_service_routing, outbound_device},
                  handle::service::conversation::connect::Request< outbound_device_type>{ m_routing, outbound_device},

                  handle::queue::enqueue::Request< outbound_device_type>{ m_routing, outbound_device},
                  handle::queue::dequeue::Request< outbound_device_type>{ m_routing, outbound_device},

                  handle::create< common::message::transaction::resource::prepare::Request>( m_routing, outbound_device),
                  handle::create< common::message::transaction::resource::commit::Request>( m_routing, outbound_device),
                  handle::create< common::message::transaction::resource::rollback::Request>( m_routing, outbound_device),

                  handle::create< common::message::gateway::domain::discover::Request>( m_routing, outbound_device)
               );

               common::message::dispatch::pump( handler, common::communication::ipc::inbound::device(), ipc_policy{});
            }

         private:

            configuration_type connect()
            {
               Trace trace{ "gateway::outbound::Gateway::connect"};

               message::worker::Connect message;

               auto handler = common::communication::ipc::inbound::device().handler(
                  gateway::handle::Disconnect{ m_reply_thread},
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  common::message::handle::assign( message)
               );

               while( ! message.correlation)
               {
                  handler( common::communication::ipc::inbound::device().next( handler.types(), ipc_policy{}));
               }

               configuration_type configuration;

               common::marshal::binary::Input marshal{ message.information};
               marshal >> configuration;

               return configuration;
            }

            template< typename D, typename A>
            void initialize( D& device, A&& address)
            {
               Trace trace{ "gateway::outbound::Gateway::initialize"};

               //
               // Get the configuration for this outbound
               //

               auto get_configuration = [&]()
               {
                  Trace trace{ "gateway::outbound::Gateway::initialize get_configuration"};

                  message::outbound::configuration::Request request;
                  request.process = common::process::handle();

                  common::communication::ipc::blocking::send(
                        common::communication::ipc::gateway::manager::device(),
                        request);

                  //
                  // We need to be able to response to other messages so we don't block "for ever"
                  //
                  message::outbound::configuration::Reply reply;

                  auto handler = common::communication::ipc::inbound::device().handler(
                     gateway::handle::Disconnect{ m_reply_thread},
                     common::message::handle::Shutdown{},
                     common::message::handle::ping(),
                     common::message::handle::assign( reply));


                  while( ! reply.correlation)
                  {
                     handler( common::communication::ipc::inbound::device().next( handler.types(), ipc_policy{}));
                  }

                  return reply;
               };


               const auto correlation = common::uuid::make();

               //
               // Send discovery to remote
               //
               {
                  Trace trace{ "gateway::outbound::Gateway::initialize send discovery request"};

                  auto configuration = get_configuration();
                  common::message::gateway::domain::discover::Request request;

                  request.correlation = correlation;
                  request.process = common::process::handle();
                  request.domain = common::domain::identity();
                  request.services = configuration.services;
                  request.queues = configuration.queues;

                  //
                  // Use regular handler, that will advertise services and queues if found in the other domain.
                  //
                  handle::create< common::message::gateway::domain::discover::Request>( m_routing, device)( request);

               }

               common::message::gateway::domain::discover::Reply reply;

               //
               // wait for the reply
               //
               {
                  Trace trace{ "gateway::outbound::Gateway::initialize get discovery reply"};

                  //
                  // We need to be able to response to other messages so we don't block "for ever"
                  //

                  auto handler = common::communication::ipc::inbound::device().handler(
                     gateway::handle::Disconnect{ m_reply_thread},
                     common::message::handle::Shutdown{},
                     common::message::handle::ping(),
                     common::message::handle::assign( reply)
                  );

                  while( ! reply.correlation)
                  {
                     handler( common::communication::ipc::inbound::device().next( handler.types(), ipc_policy{}));
                  }


               }

               //
               // Send connect to gateway
               //
               {
                  Trace trace{ "gateway::outbound::Gateway::initialize send connect to gatewawy"};

                  message::outbound::Connect connect;
                  connect.process = common::process::handle();
                  connect.domain = reply.domain;
                  connect.address = std::forward< A>( address);

                  common::communication::ipc::blocking::send(
                        common::communication::ipc::gateway::manager::device(),
                        connect);
               }
            }

            template< typename S>
            static void reply_thread( const service::Routing& service_routing, const Routing& routing, S&& settings)
            {
               //
               // We're only interested in sig-user
               //
               common::signal::thread::scope::Mask block{ common::signal::set::filled( common::signal::Type::user)};

               Trace trace{ "gateway::outbound::Gateway::reply_thread"};


               auto send_disconnect = []( message::worker::Disconnect::Reason reason)
                  {
                     //
                     // We probably just received a signal, and we just want to shutdown
                     //
                     common::signal::thread::scope::Block block;
                     
                     try
                     {

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
                  // Keep track of the order, we need to advertise services and queues
                  //
                  auto order = settings.order;

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

                     Trace trace{ "gateway::outbound::Gateway::reply_thread main thread connect"};

                     message::worker::Connect message;

                     {
                        auto configuration = policy.configuration();
                        common::marshal::binary::Output marshal{ message.information};
                        marshal << configuration;
                     }

                     common::communication::ipc::blocking::send( common::communication::ipc::inbound::id(), message);
                  }


                  common::log::category::information << "connection established - policy: " << policy << "\n";

                  common::message::service::remote::Metric metric;
                  metric.process = common::process::handle();
                  metric.services.reserve( common::platform::batch::gateway::metrics());



                  //
                  // we start our reply-message-pump
                  //
                  auto handler = inbound.handler(

                     //
                     // External messages from the other domain
                     //
                     handle::service::call::Reply{ service_routing, metric},

                     handle::basic_reply< common::message::queue::enqueue::Reply>{ routing},
                     handle::basic_reply< common::message::queue::dequeue::Reply>{ routing},

                     handle::basic_reply< common::message::transaction::resource::prepare::Reply>{ routing},
                     handle::basic_reply< common::message::transaction::resource::commit::Reply>{ routing},
                     handle::basic_reply< common::message::transaction::resource::rollback::Reply>{ routing},

                     handle::domain::discover::Reply{ routing, order}
                  );

                  log << "start reply message pump\n";


                  while( true)
                  {
                     while( metric.services.empty())
                     {
                        // we can block
                        handler( inbound.next( inbound.policy_blocking()));
                     }

                     //
                     // consume until queue is empty or we reach batch limit
                     //
                     while( ! handler( inbound.next( inbound.policy_non_blocking()))
                           && metric.services.size() < common::platform::batch::gateway::metrics())
                     {
                        ;
                     }

                     //
                     // Send metrics to service-manager
                     //
                     common::communication::ipc::blocking::send( common::communication::ipc::service::manager::device(), metric);
                     metric.services.clear();

                  }

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

            const service::Routing m_service_routing;
            const Routing m_routing;

            std::thread m_reply_thread;
         };


      } // outbound
   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_GATEWAY_H_
