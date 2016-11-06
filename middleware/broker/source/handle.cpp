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


//
// std
//
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

         void process_exit( const common::process::lifetime::Exit& exit)
         {
            //
            // We put a dead process event on our own ipc device, that
            // will be handled later on.
            //
            common::message::domain::process::termination::Event event{ exit};
            communication::ipc::inbound::device().push( std::move( event));
         }



         namespace process
         {
            void Exit::operator () ( message_type& message)
            {
               Trace trace{ "broker::handle::process::Exit"};

               log << "message: " << message << '\n';

               m_state.remove_process( message.death.pid);
            }

         } // process



         namespace traffic
         {
            void Connect::operator () ( common::message::traffic::monitor::connect::Request& message)
            {
               Trace trace{ "broker::handle::traffic::Connect"};

               m_state.traffic.monitors.add( message.process);

               auto reply = common::message::reverse::type( message);
               ipc::device().blocking_send( message.process.queue, reply);
            }

            void Disconnect::operator () ( message_type& message)
            {
               Trace trace{ "broker::handle::monitor::Disconnect"};

               m_state.traffic.monitors.remove( message.process.pid);
            }
         }


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

               try
               {
                  auto now = platform::clock_type::now();

                  auto& service = m_state.service( message.requested);

                  auto instance = service.idle();

                  if( instance)
                  {
                     //
                     // flag it as busy.
                     //
                     instance->lock( now);

                     auto reply = common::message::reverse::type( message);
                     reply.service = service.information;
                     reply.service.traffic_monitors = m_state.traffic.monitors.get();
                     reply.state = decltype( reply.state)::idle;
                     reply.process = instance->process();

                     ipc::device().blocking_send( message.process.queue, reply);

                     ++service.lookedup;
                  }
                  else if( service.instances.empty())
                  {
                     throw state::exception::Missing{ "no instances"};
                  }
                  else
                  {
                     auto reply = common::message::reverse::type( message);
                     reply.service = service.information;
                     reply.service.traffic_monitors = m_state.traffic.monitors.get();

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

                           ipc::device().blocking_send( message.process.queue, reply);

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
                           m_state.pending.requests.push_back( std::move( message));

                           break;
                        }
                        default:
                        {
                           //
                           // All instances are busy, we stack the request
                           //
                           m_state.pending.requests.push_back( std::move( message));

                           //
                           // ...and send busy-message to caller, to set timeouts and stuff
                           //
                           reply.state = decltype( reply.state)::busy;

                           ipc::device().blocking_send( message.process.queue, reply);

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

                     ipc::device().blocking_send( message.process.queue, reply);
                  });

                  if( ! m_state.instances.remote.empty())
                  {
                     //
                     // TODO: This is somewhat fragile.
                     //
                     log << "no instances found for service: " << message.requested << " - action: ask neighbor domains\n";

                     common::message::gateway::domain::discover::Request request;
                     request.correlation = message.correlation;
                     request.domain = common::domain::identity();
                     request.process = common::process::handle();
                     request.services.push_back( message.requested);

                     ipc::device().blocking_send( communication::ipc::gateway::manager::device(), request);

                     m_state.pending.requests.push_back( std::move( message));

                     send_reply.release();
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

                  reply.domain = common::domain::identity();

                  for( const auto& s : message.services)
                  {
                     auto&& service = m_state.find_service( s);

                     if( service)
                     {
                        if( ! service->instances.local.empty())
                        {
                           //
                           // Service is local
                           //
                           reply.services.emplace_back(
                                 service->information.name,
                                 service->information.type,
                                 service->information.transaction);
                        }
                        else if( ! service->instances.remote.empty())
                        {
                           reply.services.emplace_back(
                                 service->information.name,
                                 service->information.type,
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

                  auto found = range::find_if( m_state.pending.requests, [&]( const common::message::service::lookup::Request& r){
                     return r.correlation == message.correlation;
                  });

                  if( found)
                  {
                     auto pending = std::move( *found);
                     m_state.pending.requests.erase( std::begin( found));

                     auto service = m_state.find_service( pending.requested);

                     if( service && ! service->instances.empty())
                     {
                        //
                        // The requested service is now available, use
                        // the lookup to decide how to progress.
                        //
                        service::Lookup{ m_state}( pending);
                     }
                     else
                     {
                        auto reply = common::message::reverse::type( pending);
                        reply.service.name = pending.requested;
                        reply.state = decltype( reply.state)::absent;

                        ipc::device().blocking_send( pending.process.queue, reply);
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

            try
            {
               //
               // This message can only come from a local instance
               //

               auto& instance = m_state.local_instance( message.process.pid);

               auto now = platform::clock_type::now();

               instance.unlock( now);

               //
               // Check if there are pending request for services that this
               // instance has.
               //

               {
                  //
                  // TODO: make this more efficient...
                  //

                  auto pending = common::range::find_if( m_state.pending.requests, [&]( const common::message::service::lookup::Request& r){
                     auto service = m_state.find_service( r.requested);

                     return service && range::find( service->instances.local, message.process.pid);
                  });

                  if( pending)
                  {

                     //
                     // We now know that there are one idle server that has advertised the
                     // requested service (we've just marked it as idle...).
                     // We can use the normal request to get the response
                     //
                     service::Lookup lookup( m_state);
                     lookup( *pending);

                     //
                     // Remove pending
                     //
                     m_state.pending.requests.erase( std::begin( pending));
                  }
               }
            }
            catch( state::exception::Missing& exception)
            {
               common::log::error << "failed to find instance on ACK - indicate inconsistency - action: ignore\n";
            }
         }


         void Policy::connect( std::vector< common::message::service::advertise::Service> services, const std::vector< common::transaction::Resource>& resources)
         {
            m_state.connect_broker( std::move( services));
         }


         void Policy::reply( platform::ipc::id::type id, common::message::service::call::Reply& message)
         {
            ipc::device().blocking_send( id, message);
         }

         void Policy::ack( const common::message::service::call::callee::Request& message)
         {
            common::message::service::call::ACK ack;

            ack.process = common::process::handle();
            ack.service = message.service.name;

            ACK sendACK( m_state);
            sendACK( ack);
         }

         void Policy::transaction( const common::message::service::call::callee::Request&, const server::Service&, const common::platform::time_point&)
         {
            // broker doesn't bother with transactions...
         }

         void Policy::transaction( const common::message::service::call::Reply& message, int return_state)
         {
            // broker doesn't bother with transactions...
         }

         void Policy::forward( const common::message::service::call::callee::Request& message, const common::server::State::jump_t& jump)
         {
            throw common::exception::xatmi::System{ "can't forward within broker"};
         }

         void Policy::statistics( platform::ipc::id::type id,common::message::traffic::Event&)
         {
            //
            // We don't collect statistics for the broker
            //
         }

      } // handle

      handle::dispatch_type handler( State& state)
      {
         return {
            handle::process::Exit{ state},
            handle::service::Advertise{ state},
            handle::service::Lookup{ state},
            handle::ACK{ state},
            handle::traffic::Connect{ state},
            handle::traffic::Disconnect{ state},
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
