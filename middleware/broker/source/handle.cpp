//!
//! handle.cpp
//!
//! Created on: Jun 1, 2014
//!     Author: Lazan
//!

#include "broker/handle.h"
#include "broker/transform.h"
#include "broker/admin/server.h"

#include "common/server/lifetime.h"
#include "common/internal/log.h"
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
#include <fstream>

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





         /*
         std::vector< common::platform::pid::type> spawn( const State& state, const state::Executable& executable, std::size_t instances)
         {
            std::vector< common::platform::pid::type> pids;

            //
            // Prepare environment. We use the default first and add
            // specific for the executable
            //
            auto environment = state.standard.environment;
            environment.insert(
               std::end( environment),
               std::begin( executable.environment.variables),
               std::end( executable.environment.variables));

            while( instances-- > 0)
            {
               pids.push_back( common::process::spawn( executable.path, executable.arguments, environment));
            }

            return pids;
         }
         */



         namespace traffic
         {
            void Connect::operator () ( common::message::traffic::monitor::connect::Request& message)
            {
               trace::internal::Scope trace{ "broker::handle::monitor::Connect"};

               m_state.traffic.monitors.add( message.process);
            }

            void Disconnect::operator () ( message_type& message)
            {
               trace::internal::Scope trace{ "broker::handle::monitor::Disconnect"};

               m_state.traffic.monitors.remove( message.process.pid);
            }
         }




         namespace forward
         {

            void Connect::operator () ( const common::message::forward::connect::Request& message)
            {
               common::trace::internal::Scope trace{ "broker::handle::forward::Connect"};

               m_state.forward = message.process;
            }

         } // forward


         void Advertise::operator () ( message_type& message)
         {
            trace::internal::Scope trace{ "broker::handle::Advertise"};

            m_state.add( message.process, message.services);
         }

         void Unadvertise::operator () ( message_type& message)
         {
            trace::internal::Scope trace{ "broker::handle::Unadvertise"};

            m_state.remove( message.process, message.services);
         }


         namespace service
         {

            void Lookup::operator () ( message_type& message)
            {
               trace::internal::Scope trace{ "broker::handle::service::Lookup"};

               try
               {
                  auto& service = m_state.service( message.requested);

                  auto instance = service.idle();

                  if( instance)
                  {
                     //
                     // flag it as busy.
                     //
                     instance->state( state::Instance::State::busy);

                     auto reply = common::message::reverse::type( message);
                     reply.service = service.information;
                     reply.service.traffic_monitors = m_state.traffic.monitors.get();
                     reply.state = decltype( reply.state)::idle;
                     reply.process = instance->instance.get().process;

                     ipc::device().blocking_send( message.process.queue, reply);

                     service.lookedup++;
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
               catch( state::exception::Missing& exception)
               {
                  //
                  // TODO: We will send the request to the gateway. (only if we want auto discovery)
                  //
                  // Server (queue) that hosts the requested service is not found.
                  // We propagate this by having 0 occurrence of server in the response
                  //
                  auto reply = common::message::reverse::type( message);
                  reply.service.name = message.requested;
                  reply.state = decltype( reply.state)::absent;

                  ipc::device().blocking_send( message.process.queue, reply);
               }
            }

         } // service



         void ACK::operator () ( message_type& message)
         {
            trace::internal::Scope trace{ "broker::handle::ACK"};

            try
            {
               auto& instance = m_state.instance( message.process.pid);

               instance.state( state::Instance::State::idle);
               ++instance.invoked;

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
                     if( service)
                     {
                        return range::any_of( service->instances, [&]( const state::Service::Instance& i){
                           return i.process() == message.process;
                        });
                     }
                     return false;
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


         void Policy::connect( std::vector< common::message::Service> services, const std::vector< common::transaction::Resource>& resources)
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

      common::message::dispatch::Handler handler( State& state)
      {
         return {

            handle::forward::Connect{ state},
            //handle::dead::process::Event{ state},
            handle::Advertise{ state},
            handle::Unadvertise{ state},
            handle::service::Lookup{ state},
            handle::ACK{ state},
            handle::traffic::Connect{ state},
            handle::traffic::Disconnect{ state},
            handle::Call{ admin::services( state), state},
            common::message::handle::Ping{},
            common::message::handle::Shutdown{},
         };
      }

      common::message::dispatch::Handler handler_no_services( State& state)
      {
         return {
            handle::forward::Connect{ state},
            //handle::dead::process::Event{ state},
            handle::Advertise{ state},
            handle::Unadvertise{ state},
            handle::service::Lookup{ state},
            handle::ACK{ state},
            handle::traffic::Connect{ state},
            handle::traffic::Disconnect{ state},
            //handle::Call{ communication::ipc::inbound::device(), admin::services( state), state},
            common::message::handle::Ping{},
            common::message::handle::Shutdown{},
         };
      }
   } // broker
} // casual
