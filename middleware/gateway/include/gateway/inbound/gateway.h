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
#include "common/message/coordinate.h"
#include "common/message/handle.h"

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

            namespace optional
            {
               template< typename D, typename M>
               bool send( D&& device, M&& message)
               {
                  try
                  {
                     blocking::send( std::forward< D>( device), std::forward< M>( message));
                     return true;
                  }
                  catch( const common::exception::communication::Unavailable&)
                  {
                     return false;
                  }
               }
            } // optional
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
                  // Set 'sender' so we (our main thread) get the reply
                  //
                  message.process = common::process::handle();
                  blocking::send( common::communication::ipc::transaction::manager::device(), message.get());
               }
            };

            namespace cache
            {
               struct Base
               {
                  Base( const Cache& cache) : m_cache( cache) {}

               protected:
                  const Cache& m_cache;
               };

            } // cache

            namespace call
            {
               struct Request : cache::Base
               {
                  using message_type = message::interdomain::service::call::receive::Request;

                  using cache::Base::Base;


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
                     m_cache.add( common::marshal::complete( std::move( message.get())));

                     //
                     // Send lookup
                     //
                     blocking::send( common::communication::ipc::service::manager::device(), request);
                  }
               };

               namespace lookup
               {
                  struct Reply : cache::Base
                  {
                     using message_type = common::message::service::lookup::Reply;

                     using cache::Base::Base;

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
                                 common::log::category::error << "server: " << message.process << " has been terminated during interdomain call - action: reply with TPESVCERR\n";
                                 send_error_reply( message);
                              }
                              break;
                           }
                           case message_type::State::absent:
                           {
                              common::log::category::error << "service: " << message.service << " is not handled by this domain (any more) - action: reply with TPESVCERR\n";
                              send_error_reply( message);
                              break;
                           }
                           default:
                           {
                              common::log::category::error << "unexpected state on lookup reply: " << message << " - action: drop message\n";
                              break;
                           }
                        }
                     }

                  private:
                     void send_error_reply( message_type& message)
                     {
                        common::message::service::call::Reply reply;
                        reply.correlation = message.correlation;
                        reply.status = TPESVCERR;
                        common::communication::ipc::inbound::device().push( reply);
                     }
                  };

               } // lookup
            } // call

            namespace queue
            {
               namespace lookup
               {
                  template< typename M>
                  void send( const Cache& cache, M&& message)
                  {
                     Trace trace{ "gateway::inbound::handle::queue::lookup::send"};

                     //
                     // Change 'sender' so we (our main thread) get the reply
                     //
                     message.process = common::process::handle();

                     //
                     // Prepare queue lookup
                     //
                     common::message::queue::lookup::Request request;
                     {
                        request.correlation = message.correlation;
                        request.name = message.name;
                        request.process = common::process::handle();
                     }

                     //
                     // Add message to cache, this could block
                     //
                     cache.add( common::marshal::complete( std::move( message.get())));

                     auto remove = common::scope::execute( [&](){
                        cache.get( request.correlation);
                     });

                     //
                     // Send lookup
                     //

                     blocking::send( common::communication::ipc::queue::manager::optional::device(), request);

                     //
                     // We could send the lookup, so we won't remove the message from the cache
                     //
                     remove.release();
                  }

                  struct Reply : cache::Base
                  {
                     using message_type = common::message::queue::lookup::Reply;
                     using cache::Base::Base;


                     void operator() ( message_type& message) const
                     {
                        Trace trace{ "gateway::inbound::handle::queue::lookup::Reply"};

                        auto request = m_cache.get( message.correlation);

                        if( message.process)
                        {
                           common::communication::ipc::outbound::Device ipc{ message.process.queue};
                           ipc.put( request, common::communication::ipc::policy::Blocking{});
                        }
                        else
                        {
                           send_error_reply( message, request.type);
                        }

                     }

                  private:

                     template< typename R>
                     void send_error_reply( message_type& message) const
                     {
                        R reply;
                        reply.correlation = message.correlation;
                        common::communication::ipc::inbound::device().push( reply);
                     }

                     void send_error_reply( message_type& message, common::message::Type type) const
                     {
                        switch( type)
                        {
                           case common::message::Type::queue_dequeue_request:
                           {
                              send_error_reply< common::message::queue::dequeue::Reply>( message);
                              break;
                           }
                           case common::message::Type::queue_enqueue_request:
                           {
                              send_error_reply< common::message::queue::enqueue::Reply>( message);
                              break;
                           }
                           default:
                           {
                              common::log::category::error << "unexpected message type for queue request: " << message << " - action: drop message\n";
                           }
                        }
                     }

                  };

               } // lookup

               namespace enqueue
               {
                  struct Request : cache::Base
                  {
                     using message_type = message::interdomain::queue::enqueue::receive::Request;

                     using cache::Base::Base;

                     void operator() ( message_type& message) const
                     {
                        Trace trace{ "gateway::inbound::handle::queue::enqueue::Request::operator()"};

                        //
                        // Send lookup
                        //
                        try
                        {
                           queue::lookup::send( m_cache, message);
                        }
                        catch( const common::exception::communication::Unavailable&)
                        {
                           common::log::category::error << "failed to lookup queue - action: send error reply\n";

                           common::message::queue::enqueue::Reply reply;
                           reply.correlation = message.correlation;
                           reply.execution = message.execution;

                           // empty uuid represent error. TODO: is this enough?
                           reply.id = common::uuid::empty();

                           blocking::send( common::communication::ipc::inbound::id(), reply);
                        }
                     }

                  };

               } // enqueue

               namespace dequeue
               {
                  struct Request : cache::Base
                  {
                     using message_type = message::interdomain::queue::dequeue::receive::Request;

                     using cache::Base::Base;

                     void operator() ( message_type& message) const
                     {
                        Trace trace{ "gateway::inbound::handle::queue::dequeue::Request::operator()"};

                        //
                        // Send lookup
                        //
                        try
                        {
                           queue::lookup::send( m_cache, message);
                        }
                        catch( const common::exception::communication::Unavailable&)
                        {
                           common::log::category::error << "failed to lookup queue - action: send error reply\n";

                           common::message::queue::enqueue::Reply reply;
                           reply.correlation = message.correlation;
                           reply.execution = message.execution;

                           // empty uuid represent error. TODO: is this enough?
                           reply.id = common::uuid::empty();

                           blocking::send( common::communication::ipc::inbound::id(), reply);
                        }
                     }

                  };
               } // dequeue
            } // queue


            template< typename M, typename D>
            struct basic_forward
            {
               using message_type = M;
               using device_type = D;

               basic_forward( device_type& device) : m_device( device) {}

               void operator() ( message_type& message)
               {
                  Trace trace{ "gateway::inbound::handle::basic_forward::operator()"};

                  auto&& wrapper = message::interdomain::send::wrap( message);

                  log << "forward message: " << message << '\n';

                  m_device.blocking_send( wrapper);
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

                  namespace coordinate
                  {
                     //!
                     //! Policy to coordinate discover request from another domain
                     //!
                     template< typename D>
                     struct Policy
                     {
                        using device_type = D;

                        using message_type = common::message::gateway::domain::discover::Reply;

                        Policy( device_type& device) : m_device( device) {}

                        inline void accumulate( message_type& message, message_type& reply)
                        {
                           Trace trace{ "gateway::inbound::handle::domain::discover::coordinate::Policy::accumulate"};


                           common::range::copy( reply.services, std::back_inserter( message.services));
                           common::range::copy( reply.queues, std::back_inserter( message.queues));

                           log << "reply: " << reply << '\n';
                           log << "message: " << message << '\n';

                        }

                        inline void send( common::communication::ipc::Handle queue, message_type& message)
                        {
                           Trace trace{ "gateway::inbound::handle::domain::discover::coordinate::Policy::send"};

                           //
                           // Set our domain-id to the reply so the outbound can deduce stuff
                           //
                           message.domain = common::domain::identity();
                           message.process = common::process::handle();

                           auto&& wrapper = gateway::message::interdomain::send::wrap( message);

                           log << "forward message: " << message << '\n';

                           m_device.blocking_send( wrapper);
                        }
                     private:
                        device_type& m_device;
                     };

                     template< typename D>
                     using Discover = common::message::Coordinate< Policy< D>>;

                  } // coordinate

                  namespace forward
                  {
                     struct Request
                     {
                        void operator () ( message::interdomain::domain::discovery::receive::Request& message) const
                        {
                           Trace trace{ "gateway::inbound::handle::connection::discover::forward::Request"};

                           log << "message: " << message << '\n';

                           //
                           // Request from remote domain about this domain
                           // We forward it to main thread (for async ordering reasons)
                           //

                           blocking::send( common::communication::ipc::inbound::id(), message.get());
                        }
                     };
                  } // forward

                  template< typename D>
                  struct Request
                  {
                     using discover_type = D;

                     Request( discover_type& device) : m_discover( device) {}

                     void operator () ( common::message::gateway::domain::discover::Request& message) const
                     {
                        Trace trace{ "gateway::inbound::handle::connection::discover::Request"};

                        log << "message: " << message << '\n';

                        //
                        // Make sure we gets the reply
                        //
                        message.process = common::process::handle();


                        //
                        // Forward to broker and possible casual-queue
                        //
                        std::vector< common::platform::pid::type> pids;

                        if( ! message.services.empty())
                        {
                           blocking::send( common::communication::ipc::service::manager::device(), message);
                           pids.push_back( common::communication::ipc::service::manager::device().connector().process().pid);
                        }

                        if( ! message.queues.empty() &&
                              blocking::optional::send( common::communication::ipc::queue::manager::optional::device(), message))
                        {
                           pids.push_back( common::communication::ipc::queue::manager::optional::device().connector().process().pid);
                        }

                        m_discover.add( message.correlation, {}, pids);
                     }

                  private:
                     discover_type& m_discover;
                  };

                  namespace request
                  {
                     template< typename C>
                     auto make( C&& coordinate) -> Request< std::remove_reference_t< C>>
                     {
                        return { std::forward< C>( coordinate)};
                     }
                  } // coordinate


                  template< typename D>
                  struct Reply
                  {
                     using discover_type = coordinate::Discover< D>;
                     using message_type = common::message::gateway::domain::discover::Reply;

                     Reply( discover_type& device) : m_discover( device) {}

                     void operator() ( message_type& message)
                     {
                        Trace trace{ "gateway::inbound::handle::domain::discover::Reply::operator()"};

                        log << "message: " << message << '\n';

                        //
                        // Might send the accumulated message if all requested has replied.
                        // (via the Policy)
                        //
                        m_discover.accumulate( message);
                     }

                  private:
                     discover_type& m_discover;

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
              : m_cache{ policy_type::limits( settings)}
            {
               //
               // 'connect' to our local domain
               //
               common::process::instance::connect();

               //
               // Start worker thread
               //
               m_request_thread = std::thread{ request_thread< S>, std::ref( m_cache), std::forward< S>( settings)};
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



               using outbound_type = common::traits::concrete::type_t< decltype( outbound_device)>;

               //
               // Coordinates discover replies
               //
               handle::domain::discover::coordinate::Discover< outbound_type> discover{ outbound_device};


               //
               // Initialize and connect to gateway
               //
               initialize( discover, policy.address( outbound_device));



               auto handler = common::communication::ipc::inbound::device().handler(
                  //
                  // Internal messages
                  //
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  gateway::handle::Disconnect{ m_request_thread},
                  handle::call::lookup::Reply{ m_cache},
                  handle::queue::lookup::Reply{ m_cache},

                  //
                  // External messages that will be forward to the other domain
                  //
                  handle::create::forward< common::message::service::call::Reply>( outbound_device),
                  handle::create::forward< common::message::transaction::resource::prepare::Reply>( outbound_device),
                  handle::create::forward< common::message::transaction::resource::commit::Reply>( outbound_device),
                  handle::create::forward< common::message::transaction::resource::rollback::Reply>( outbound_device),

                  //
                  // accumulates discover replies, and forward the accumulated message when all has
                  // replied
                  //
                  handle::domain::discover::Reply< outbound_type>{ discover},
                  handle::domain::discover::request::make( discover),

                  //
                  // Queue replies
                  //
                  handle::create::forward< common::message::queue::dequeue::Reply>( outbound_device),
                  handle::create::forward< common::message::queue::enqueue::Reply>( outbound_device)
               );

               log << "start internal message pump\n";
               common::message::dispatch::pump( handler, common::communication::ipc::inbound::device(), ipc_policy{});

            }

         private:

            configuration_type connect()
            {
               Trace trace{ "gateway::inbound::Gateway::connect"};

               message::worker::Connect message;

               auto handler = common::communication::ipc::inbound::device().handler(
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  gateway::handle::Disconnect{ m_request_thread},
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
            void initialize( D&& discover, A&& address)
            {
               Trace trace{ "gateway::inbound::Gateway::initialize"};

               //
               // We wait for the remote outbound domain to send discover, and for our inbound thread to
               // forward it.
               // This is part of the outbound initialization dance, and we use it to initialize the inbound
               // part as well.
               // The objective is to get the remote-id
               //

               common::message::gateway::domain::discover::Request message;

               auto handler = common::communication::ipc::inbound::device().handler(
                  common::message::handle::Shutdown{},
                  common::message::handle::ping(),
                  gateway::handle::Disconnect{ m_request_thread},
                  common::message::handle::assign( message)
               );

               while( ! message.correlation)
               {
                  handler( common::communication::ipc::inbound::device().next( handler.types(), ipc_policy{}));
               }

               //
               // Send connect to gateway
               //
               {
                  Trace trace{ "gateway::inbound::Gateway::operator() gateway connect"};

                  message::inbound::Connect connect;
                  connect.domain = message.domain;
                  connect.process = common::process::handle();
                  connect.address = std::forward< A>( address);
                  common::communication::ipc::blocking::send( common::communication::ipc::gateway::manager::device(), connect);
               }

               //
               // Handle the message, so the remote outbound gets what it wants.
               //
               {
                  handle::domain::discover::request::make( discover)( message);
               }
            }

            template< typename S>
            static void request_thread( const Cache& cache, S&& settings)
            {
               //
               // We're only interested in sig-user
               //
               common::signal::thread::scope::Mask block{ common::signal::set::filled( common::signal::Type::user)};


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

                  common::log::category::information << "connection established - policy: " << policy << "\n";


                  //
                  // we start our request-message-pump
                  //
                  auto handler = device.handler(
                     handle::call::Request{ cache},
                     handle::basic_transaction_request< message::interdomain::transaction::resource::receive::prepare::Request>{},
                     handle::basic_transaction_request< message::interdomain::transaction::resource::receive::commit::Request>{},
                     handle::basic_transaction_request< message::interdomain::transaction::resource::receive::rollback::Request>{},
                     handle::domain::discover::forward::Request{},

                     handle::queue::dequeue::Request{ cache},
                     handle::queue::enqueue::Request{ cache}
                  );
             
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
