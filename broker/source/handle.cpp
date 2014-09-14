//!
//! handle.cpp
//!
//! Created on: Jun 1, 2014
//!     Author: Lazan
//!

#include "broker/handle.h"
#include "broker/transform.h"
#include "broker/filter.h"

#include "common/queue.h"
#include "common/internal/log.h"
#include "common/environment.h"
#include "common/algorithm.h"
#include "common/process.h"


//
// std
//
#include <vector>
#include <string>

namespace casual
{

   namespace broker
   {

      namespace handle
      {

         void MonitorConnect::dispatch( message_type& message)
         {
            //TODO: Temp
            m_state.monitorQueue = message.server.queue_id;
         }

         void MonitorDisconnect::dispatch( message_type& message)
         {
            m_state.monitorQueue = 0;
         }


         namespace transaction
         {

            void ManagerConnect::dispatch( message_type& message)
            {
               m_state.transactionManagerQueue = message.server.queue_id;

               auto& instance = m_state.getInstance( message.server.pid);
               instance.alterState( state::Server::Instance::State::idle);

               //
               // Send configuration to TM
               //
               auto configuration = transform::transaction::configuration( m_state);

               QueueBlockingWriter tmQueue{ message.server.queue_id, m_state};
               tmQueue( configuration);

            }


            namespace client
            {

               void Connect::dispatch( message_type& message)
               {

                  common::trace::internal::Scope trace{ "broker::handle::transaction::client::Connect::dispatch"};

                  common::log::internal::debug << "connect request: " << message.server << std::endl;

                  try
                  {
                     auto& instance = m_state.getInstance( message.server.pid);

                     //
                     // Instance is started for the first time.
                     // Send some configuration
                     //
                     message::transaction::client::connect::Reply reply =
                           transform::transaction::client::reply( m_state, instance);

                     QueueBlockingWriter write( message.server.queue_id, m_state);
                     write( reply);
                  }
                  catch( const state::exception::Missing& exception)
                  {
                     // What to do? Add the instance?
                     log::error << "could not find instance - " << exception.what() << std::endl;
                  }
               }
            } // client
         } // transaction


         void Advertise::dispatch( message_type& message)
         {

            std::vector< state::Service> services;

            common::range::transform( message.services, services, transform::Service{});

            m_state.addServices( message.server.pid, std::move( services));

         }

         void Unadvertise::dispatch( message_type& message)
         {
            std::vector< state::Service> services;

            common::range::transform( message.services, services, transform::Service{});

            m_state.removeServices( message.server.pid, std::move( services));
         }

         void Disconnect::dispatch( message_type& message)
         {
            //
            // Remove the instance
            //
            m_state.removeInstance( message.server.pid);
         }

         template< typename M>
         void Disconnect::dispatch( M& message)
         {
            //
            // Remove the instance
            //
            m_state.removeInstance( message.server.pid);

            // TODO: We have to check if this affect pending...
         }


         void Connect::dispatch( message_type& message)
         {
            common::trace::internal::Scope trace{ "broker::handle::Connect::dispatch"};

            common::log::internal::debug << "connect request: " << message.server << std::endl;

            try
            {

               auto& instance = m_state.getInstance( message.server.pid);

               //
               // Instance is started for the first time.
               // Send some configuration
               //
               message::server::connect::Reply reply;

               common::log::internal::debug << "connect reply: " << message.server << std::endl;

               QueueBlockingWriter writer( message.server.queue_id, m_state);
               writer( reply);


               //
               // Add services
               //
               {
                  std::vector< state::Service> services;

                  common::range::transform( message.services, services, transform::Service{});

                  m_state.addServices( message.server.pid, std::move( services));
               }

               //
               // Set the instance to idle state
               //
               instance.alterState( state::Server::Instance::State::idle);

            }
            catch( const state::exception::Missing& exception)
            {
               //
               // The instance was started outside the broker. This is totally in order, though
               // there will be no 'instances' semantics, hence limited administration possibilities.
               // We add it...
               //
               m_state.instances.emplace(
                     message.server.pid, broker::transform::Instance()( message));

               dispatch( message);
            }
         }


         //typedef basic_connect< broker::QueueBlockingWriter> Connect;





         void ServiceLookup::dispatch( message_type& message)
         {

            try
            {
               auto& service = m_state.getService( message.requested);

               //
               // Try to find an idle instance.
               //
               auto idle = common::range::find_if(
                     service.instances,
                     filter::instance::Idle{});


               if( idle)
               {
                  //
                  // flag it as busy.
                  //
                  idle->get().alterState( state::Server::Instance::State::busy);

                  message::service::name::lookup::Reply reply;
                  reply.service = service.information;
                  reply.service.monitor_queue = m_state.monitorQueue;
                  reply.server.push_back( transform::Instance()( *idle));

                  QueueBlockingWriter writer( message.server.queue_id, m_state);
                  writer( reply);

                  service.lookedup++;
               }
               else
               {
                  //
                  // All servers are busy, we stack the request
                  //
                  m_state.pending.push_back( std::move( message));
               }

            }
            catch( state::exception::Missing& exception)
            {
               //
               // TODO: We will send the request to the gateway.
               //
               // Server (queue) that hosts the requested service is not found.
               // We propagate this by having 0 occurrence of server in the response
               //
               message::service::name::lookup::Reply reply;
               reply.service.name = message.requested;

               QueueBlockingWriter writer( message.server.queue_id, m_state);
               writer( reply);
            }
         }


         void ACK::dispatch( message_type& message)
         {
            try
            {
               auto& instance = m_state.getInstance( message.server.pid);

               instance.alterState( state::Server::Instance::State::idle);
               ++instance.invoked;

               //
               // Check if there are pending request for services that this
               // instance has.
               //
               {
                  auto pending = common::range::find_if(
                     common::range::make( m_state.pending),
                     filter::Pending( instance));

                  if( pending)
                  {

                     //
                     // We now know that there are one idle server that has advertised the
                     // requested service (we've just marked it as idle...).
                     // We can use the normal request to get the response
                     //
                     ServiceLookup lookup( m_state);
                     lookup.dispatch( *pending);

                     //
                     // Remove pending
                     //
                     m_state.pending.erase( pending.first);
                  }

               }
            }
            catch( state::exception::Missing& exception)
            {
               common::log::error << "failed to find instance on ACK - indicate inconsistency " << exception.what() <<std::endl;
            }
         }


         void Policy::connect(  message::server::connect::Request& message, const std::vector< common::transaction::Resource>& resources)
         {

            message.server.queue_id = ipc::receive::id();
            message.path = common::process::path();

            //
            // We add the server
            //
            auto& instance = m_state.addInstance( transform::Instance()( message));

            //
            // Add services
            //
            {
               std::vector< state::Service> services;

               common::range::transform( message.services, services, transform::Service{});

               m_state.addServices( message.server.pid, std::move( services));
            }

            instance.alterState( state::Server::Instance::State::idle);

         }

         void Policy::disconnect()
         {
            message::server::Disconnect message;

            Disconnect disconnect( m_state);
            disconnect.dispatch( message);
         }

         void Policy::reply( platform::queue_id_type id, message::service::Reply& message)
         {
            QueueBlockingWriter writer( id, m_state);
            writer( message);
         }

         void Policy::ack( const message::service::callee::Call& message)
         {
            message::service::ACK ack;
            ack.server.queue_id = ipc::receive::id();
            ack.service = message.service.name;

            ACK sendACK( m_state);
            sendACK.dispatch( ack);
         }

         void Policy::transaction( const message::service::callee::Call&, const server::Service&)
         {
            // broker doesn't bother with transactions...
         }

         void Policy::transaction( const message::service::Reply& message)
         {
            // broker doesn't bother with transactions...
         }

         void Policy::statistics( platform::queue_id_type id, message::monitor::Notify& message)
         {
            //
            // We don't collect statistics for the broker
            //
         }



      } // handle
   } // broker
} // casual
