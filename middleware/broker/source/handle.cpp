//!
//! casual
//!

#include "broker/handle.h"
#include "broker/transform.h"
#include "broker/admin/server.h"
#include "broker/common.h"

#include "common/server/lifetime.h"
#include "common/environment.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/event/listen.h"


#include <vector>
#include <string>


namespace casual
{
   using namespace common;

   namespace broker
   {

      namespace ipc
      {
         const common::communication::ipc::Helper& device()
         {
            static communication::ipc::Helper singleton{ communication::error::handler::callback::on::Terminate{ &handle::process_exit}};
            return singleton;
         }
      } // ipc

      namespace handle
      {
         namespace local
         {
            namespace
            {
               namespace optional
               {
                  template< typename D, typename M>
                  bool send( D&& device, M&& message)
                  {
                     try
                     {
                        ipc::device().blocking_send( device, message);
                        return true;
                     }
                     catch( const common::exception::communication::Unavailable&)
                     {
                        return false;
                     }
                  }
               } // optional
            } // <unnamed>
         } // local


         void process_exit( const common::process::lifetime::Exit& exit)
         {
            //
            // We put a dead process event on our own ipc device, that
            // will be handled later on.
            //
            common::message::event::process::Exit event{ exit};
            communication::ipc::inbound::device().push( std::move( event));
         }



         namespace process
         {
            void Exit::operator () ( message_type& message)
            {
               Trace trace{ "broker::handle::process::Exit"};

               log << "message: " << message << '\n';

               m_state.remove_process( message.state.pid);
            }

            namespace prepare
            {
               void Shutdown::operator () ( common::message::domain::process::prepare::shutdown::Request& message)
               {
                  Trace trace{ "broker::handle::process::prepare::Shutdown"};

                  auto reply = common::message::reverse::type( message);
                  reply.process = common::process::handle();
                  reply.processes = std::move( message.processes);

                  range::for_each( reply.processes, [&]( const auto& p){
                     m_state.prepare_shutdown( p.pid);
                  });


                  ipc::device().blocking_send( message.process.queue, reply);
               }

            } // prepare

         } // process


         namespace event
         {
            namespace subscription
            {
               void Begin::operator () ( common::message::event::subscription::Begin& message)
               {
                  Trace trace{ "broker::handle::event::subscription::Begin"};

                  m_state.events.subscription( message);
               }

               void End::operator () ( message_type& message)
               {
                  Trace trace{ "broker::handle::event::subscription::End"};

                  m_state.events.subscription( message);
               }


            } // subscription

         } // event


         namespace service
         {
            void Advertise::operator () ( message_type& message)
            {
               Trace trace{ "broker::handle::Advertise"};

               log << "message: " << message << '\n';

               m_state.update( message);
            }


            void Lookup::operator () ( message_type& message)
            {
               Trace trace{ "broker::handle::service::Lookup"};

               auto now = platform::time::clock::type::now();

               try
               {
                  auto& service = m_state.service( message.requested);

                  auto handle = service.reserve( now);

                  if( handle)
                  {

                     auto reply = common::message::reverse::type( message);
                     reply.service = service.information;
                     reply.service.event_subscribers = m_state.subscribers();
                     reply.state = decltype( reply.state)::idle;
                     reply.process = handle;

                     if( local::optional::send( message.process.queue, reply))
                     {
                        //
                        // flag it as busy.
                        //
                        //instance->lock( now);
                     }
                  }
                  else if( ! service.instances.active())
                  {
                     throw state::exception::Missing{ "no instances"};
                  }
                  else
                  {
                     auto reply = common::message::reverse::type( message);
                     reply.service = service.information;
                     reply.service.event_subscribers = m_state.subscribers();

                     switch( message.context)
                     {
                        case common::message::service::lookup::Request::Context::no_reply:
                        case common::message::service::lookup::Request::Context::forward:
                        {
                           //
                           // The intention is "send and forget", or a plain forward, we use our forward-cache for this
                           //
                           reply.process = m_state.forward;

                           //
                           // Caller will think that service is idle, that's the whole point
                           // with our forward.
                           //
                           reply.state = decltype( reply.state)::idle;

                           local::optional::send( message.process.queue, reply);

                           break;
                        }
                        case common::message::service::lookup::Request::Context::gateway:
                        {
                           //
                           // the request is from some other domain, we'll wait until
                           // the service is idle. That is, we don't need to send timeout and
                           // stuff to the gateway, since the other domain has provided this to
                           // the caller (which of course can differ from our timeouts, if operation
                           // has change the timeout, TODO: something we need to address? probably not,
                           // since we can't make it 100% any way...)
                           //
                           m_state.pending.requests.emplace_back( std::move( message), now);

                           break;
                        }
                        default:
                        {
                           //
                           // send busy-message to caller, to set timeouts and stuff
                           //
                           reply.state = decltype( reply.state)::busy;

                           if( local::optional::send( message.process.queue, reply))
                           {
                              //
                              // All instances are busy, we stack the request
                              //
                              m_state.pending.requests.emplace_back( std::move( message), now);
                           }
                           break;
                        }
                     }
                  }
               }
               catch( const state::exception::Missing&)
               {

                  auto send_reply = scope::execute( [&](){

                     log << "no instances found for service: " << message.requested << '\n';

                     //
                     // Server (queue) that hosts the requested service is not found.
                     // We propagate this by having absent state
                     //
                     auto reply = common::message::reverse::type( message);
                     reply.service.name = message.requested;
                     reply.state = decltype( reply.state)::absent;

                     local::optional::send( message.process.queue, reply);
                  });

                  try
                  {

                     common::message::gateway::domain::discover::Request request;
                     request.correlation = message.correlation;
                     request.domain = common::domain::identity();
                     request.process = common::process::handle();
                     request.services.push_back( message.requested);

                     //
                     // If there is no gateway, this will throw
                     //
                     ipc::device().blocking_send( communication::ipc::gateway::manager::optional::device(), request);

                     log << "no instances found for service: " << message.requested << " - action: ask neighbor domains\n";

                     m_state.pending.requests.emplace_back( std::move( message), now);

                     send_reply.release();
                  }
                  catch( ...)
                  {
                     error::handler();
                  }
               }
            }

         } // service


         namespace domain
         {
            void Advertise::operator () ( message_type& message)
            {
               Trace trace{ "broker::handle::gateway::Advertise"};

               log << "message: " << message << '\n';

               m_state.update( message);
            }

            namespace discover
            {
               void Request::operator () ( message_type& message)
               {
                  Trace trace{ "broker::handle::domain::discover::Request"};

                  auto reply = common::message::reverse::type( message);

                  reply.process = common::process::handle();
                  reply.domain = common::domain::identity();

                  for( const auto& s : message.services)
                  {
                     auto&& service = m_state.find_service( s);

                     //
                     // We don't allow other domains to access or know about our
                     // admin services.
                     //
                     if( service && service->information.category != common::service::category::admin())
                     {
                        if( ! service->instances.local.empty())
                        {
                           //
                           // Service is local
                           //
                           reply.services.emplace_back(
                                 service->information.name,
                                 service->information.category,
                                 service->information.transaction);
                        }
                        else if( ! service->instances.remote.empty())
                        {
                           reply.services.emplace_back(
                                 service->information.name,
                                 service->information.category,
                                 service->information.transaction,
                                 service->instances.remote.front().hops());

                        }
                     }
                  }

                  ipc::device().blocking_send( message.process.queue, reply);
               }

               void Reply::operator () ( message_type& message)
               {
                  Trace trace{ "broker::handle::gateway::discover::Reply"};

                  auto found = range::find_if( m_state.pending.requests, [&]( const state::service::Pending& p){
                     return p.request.correlation == message.correlation;
                  });

                  if( found)
                  {
                     auto pending = std::move( *found);
                     m_state.pending.requests.erase( std::begin( found));

                     auto service = m_state.find_service( pending.request.requested);

                     if( service && ! service->instances.empty())
                     {
                        //
                        // The requested service is now available, use
                        // the lookup to decide how to progress.
                        //
                        service::Lookup{ m_state}( pending.request);
                     }
                     else
                     {
                        auto reply = common::message::reverse::type( pending.request);
                        reply.service.name = pending.request.requested;
                        reply.state = decltype( reply.state)::absent;

                        ipc::device().blocking_send( pending.request.process.queue, reply);
                     }

                  }
                  else
                  {
                     log << "failed to correlate pending request - assume it has been consumed by a recent started local server\n";
                  }
               }


            } // discover

         } // domain


         void ACK::operator () ( message_type& message)
         {
            Trace trace{ "broker::handle::ACK"};

            log << "message: " << message << '\n';

            try
            {
               auto now = platform::time::clock::type::now();

               //
               // This message can only come from a local instance
               //
               auto& instance = m_state.local( message.process.pid);
               auto service = instance.unreserve( now);

               //
               // Check if there are pending request for services that this
               // instance has.
               //

               {
                  auto pending = common::range::find_if( m_state.pending.requests, [&]( const auto& p){
                     return instance.service( p.request.requested);
                  });

                  if( pending)
                  {

                     //
                     // We now know that there are one idle server that has advertised the
                     // requested service (we've just marked it as idle...).
                     // We can use the normal request to get the response
                     //
                     service::Lookup lookup( m_state);
                     lookup( pending->request);

                     //
                     // add pending metrics
                     //
                     service->pending.add( now - pending->when);

                     //
                     // Remove pending
                     //
                     m_state.pending.requests.erase( std::begin( pending));
                  }
               }
            }
            catch( const state::exception::Missing&)
            {
               common::log::category::error << "failed to find instance on ACK - indicate inconsistency - action: ignore\n";
            }
         }


         void Policy::configure( common::server::Arguments& arguments)
         {
            m_state.connect_broker( arguments.services);
         }


         void Policy::reply( platform::ipc::id::type id, common::message::service::call::Reply& message)
         {
            ipc::device().blocking_send( id, message);
         }

         void Policy::ack()
         {
            common::message::service::call::ACK ack;

            ack.process = common::process::handle();

            ACK sendACK( m_state);
            sendACK( ack);
         }

         void Policy::transaction(
               const common::transaction::ID& trid,
               const common::server::Service& service,
               const std::chrono::microseconds& timeout,
               const common::platform::time::point::type& now)
         {
            // broker doesn't bother with transactions...
         }

         common::message::service::Transaction Policy::transaction( bool commit)
         {
            // broker doesn't bother with transactions...
            return {};
         }

         void Policy::forward( common::service::invoke::Forward&& forward, const common::message::service::call::callee::Request& message)
         {
            throw common::exception::xatmi::System{ "can't forward within broker"};
         }

         void Policy::statistics( platform::ipc::id::type, common::message::event::service::Call&)
         {
            //
            // We don't collect statistics for the broker
            //
         }

      } // handle

      handle::dispatch_type handler( State& state)
      {
         return {
            common::event::listener( handle::process::Exit{ state}),
            handle::process::prepare::Shutdown{ state},
            handle::service::Advertise{ state},
            handle::service::Lookup{ state},
            handle::ACK{ state},
            handle::event::subscription::Begin{ state},
            handle::event::subscription::End{ state},
            handle::Call{ admin::services( state), state},
            handle::domain::Advertise{ state},
            handle::domain::discover::Request{ state},
            handle::domain::discover::Reply{ state},
            common::message::handle::Ping{},
            common::message::handle::Shutdown{},
         };
      }
   } // broker
} // casual
