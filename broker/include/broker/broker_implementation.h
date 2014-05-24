//!
//! casual_broker_transform.h
//!
//! Created on: Jun 15, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_TRANSFORM_H_
#define CASUAL_BROKER_TRANSFORM_H_

#include "broker/broker.h"
#include "broker/action.h"

#include "common/message.h"
#include "common/queue.h"
#include "common/internal/log.h"
#include "common/environment.h"
#include "common/server_context.h"
#include "common/algorithm.h"

//
// std
//
#include <algorithm>
#include <vector>
#include <string>



namespace casual
{
   using namespace common;

	namespace broker
	{

	   namespace policy
      {
         struct Broker
         {
            Broker( State& state) : m_state( state) {}


            void apply()
            {
               try
               {
                  throw;
               }
               catch( const exception::signal::child::Terminate& exception)
               {
                  auto terminated = process::lifetime::ended();
                  for( auto& death : terminated)
                  {


                     switch( death.why)
                     {
                        case process::lifetime::Exit::Why::core:
                           log::error << "process crashed " << death.string() << std::endl;

                           break;
                        default:
                           log::information << "proccess died: " << death.string() << std::endl;
                           break;
                     }

                     action::remove::instance( death.pid, m_state);
                  }
               }
            }

         private:

            void clean( platform::pid_type pid)
            {
               auto findIter = m_state.instances.find( pid);

               if( findIter != std::end( m_state.instances))
               {
                  trace::Exit remove{ "remove ipc"};
                  ipc::remove( findIter->second->queue_id);

               }

            }

            State& m_state;
         };
      } // policy




	   using QueueBlockingReader = queue::blocking::basic_reader< policy::Broker>;

	   using QueueBlockingWriter = queue::blocking::basic_writer< policy::Broker>;
	   using QueueNonBlockingWriter = queue::non_blocking::basic_writer< policy::Broker>;


		namespace handle
		{
		   struct Base
		   {
		      Base( State& state) : m_state( state) {}

		   protected:
		      State& m_state;
		   };

         //!
         //! Monitor Connect
         //!
         struct MonitorConnect: public Base
         {
            typedef message::monitor::Connect message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               //TODO: Temp
               m_state.monitorQueue = message.server.queue_id;
            }
         };

         //!
         //! Monitor Disconnect
         //!
         struct MonitorDisconnect: public Base
         {
            typedef message::monitor::Disconnect message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               m_state.monitorQueue = 0;
            }

         };

         namespace transaction
         {


            //!
            //! Transaction Manager Connect
            //!
            template< typename TQ>
            struct basic_manager_connect : public Base
            {
               using queue_type = TQ;
               using message_type = message::transaction::Connect;

               using Base::Base;

               void dispatch( message_type& message)
               {
                  m_state.transactionManagerQueue = message.server.queue_id;

                  action::connect( m_state.transactionManager->instances.at( 0), message);

                  //
                  // Send configuration to TM
                  //
                  auto configuration = action::transform::transaction::configuration( m_state.groups);

                  queue_type tmQueue{ message.server.queue_id, m_state};
                  tmQueue( configuration);

               }
            };

            typedef basic_manager_connect< QueueBlockingWriter> ManagerConnect;

            namespace client
            {
               //!
               //! Transaction Client Connect
               //!
               template< typename TQ>
               struct basic_connect : public Base
               {

                  using queue_type = TQ;
                  using message_type = message::transaction::client::connect::Request;

                  using Base::Base;

                  void dispatch( message_type& message)
                  {

                     common::trace::internal::Scope trace{ "broker::handle::transaction::client::basic_connect::dispatch"};

                     common::log::internal::debug << "connect request: " << message.server << std::endl;

                     //
                     // If the instance was started by the broker, we expect to find it
                     //
                     auto foundIter =  m_state.instances.find( message.server.pid);

                     decltype( foundIter->second) instance;

                     if( foundIter != std::end( m_state.instances))
                     {
                        common::log::internal::debug << "instance found: " << message.server << std::endl;
                        instance = foundIter->second;
                     }

                     //
                     // Instance is started for the first time.
                     // Send some configuration
                     //
                     message::transaction::client::connect::Reply reply =
                           action::transform::transaction::client::reply( instance, m_state);

                     queue_type write( message.server.queue_id, m_state);
                     write( reply);

                  }
               };

               typedef basic_connect< QueueBlockingWriter> Connect;

            } // client


         } // transaction



         //!
         //! Advertise 0..N services for a server.
         //!
         struct Advertise : public Base
         {
            typedef message::service::Advertise message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               //
               // Find the instance
               //
               auto findInstance = m_state.instances.find( message.server.pid);

               if( findInstance != std::end( m_state.instances))
               {

                  //
                  // Add all the services, with this corresponding server
                  //
                  common::range::for_each(
                     common::range::make( message.services),
                     action::add::Service( m_state, findInstance->second));
               }
               else
               {
                  log::error << "server (pid: " << message.server.pid << ") has not connected before advertising services" << std::endl;

               }
            }
         };

         //!
         //! A server is disconnected
         //!
         struct Disconnect : public Base
         {
            typedef message::server::Disconnect message_type;

            using Base::Base;

            template< typename M>
            void dispatch( M& message)
            {
               //
               // Remove the instance
               //
               action::remove::instance( message.server.pid, m_state);

               // TODO: We have to check if this affect pending...
            }
         };

         template< typename Q>
         struct basic_connect : public Base
         {
            typedef message::server::connect::Request message_type;
            typedef Q queue_writer_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               common::trace::internal::Scope trace{ "broker::handle::basic_connect::dispatch"};

               common::log::internal::debug << "connect request: " << message.server << std::endl;

               //
               // If the instance was started by the broker, we expect to find it
               //
               auto instance =  m_state.instances.find( message.server.pid);

               if( instance == std::end( m_state.instances))
               {
                  //
                  // The instance was started outside the broker. This is totally in order, though
                  // there will be no 'instances' semantics, hence limited administration possibilities.
                  // We add it...
                  //

                  instance = m_state.instances.emplace(
                        message.server.pid, action::transform::Instance()( message)).first;

               }

               //
               // Instance is started for the first time.
               // Send some configuration
               //
               message::server::connect::Reply reply; //=
                     //action::transform::configuration( instance->second, m_state);

               common::log::internal::debug << "connect reply: " << message.server << std::endl;

               queue_writer_type writer( message.server.queue_id, m_state);
               writer( reply);

               //
               // Add services
               //
               common::range::for_each(
                  message.services,
                  action::add::Service( m_state, instance->second));

               //
               // Set the instance in 'ready' mode
               //
               action::connect( instance->second, message);
            }
         };

         typedef basic_connect< broker::QueueBlockingWriter> Connect;


		   //!
         //! Unadvertise 0..N services for a server.
         //!
		   struct Unadvertise : public Base
         {
            typedef message::service::Unadvertise message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               //
               // Find the instance
               //
               auto findInstance = m_state.instances.find( message.server.pid);

               if( findInstance != std::end( m_state.instances))
               {

                  //
                  // Remove services, with this corresponding server
                  //
                  common::range::for_each(
                     common::range::make( message.services),
                     action::remove::Service( m_state, findInstance->second));

               }
               else
               {
                  log::error << "server (pid: " << message.server.pid << ") has not connected before unadverties services" << std::endl;

               }
            }
         };





		   //!
         //! Looks up a service-name
         //!
         template< typename Q>
         struct basic_servicelookup : public Base
         {
            typedef message::service::name::lookup::Request message_type;

            typedef Q queue_writer_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               auto serviceFound = m_state.services.find( message.requested);

               if( serviceFound != std::end( m_state.services) && ! serviceFound->second->instances.empty())
               {
                  auto& instances = serviceFound->second->instances;

                  //
                  // Try to find an idle instance.
                  //
                  auto idleFound = common::range::find_if(
                        common::range::make( instances),
                        action::find::idle::Instance());

                  if( ! idleFound.empty())
                  {
                     //
                     // flag it as busy.
                     //
                     (*idleFound.first)->alterState( Server::Instance::State::busy);

                     message::service::name::lookup::Reply reply;
                     reply.service = serviceFound->second->information;
                     reply.service.monitor_queue = m_state.monitorQueue;
                     reply.server.push_back( action::transform::Instance()( *idleFound.first));

                     queue_writer_type writer( message.server.queue_id, m_state);
                     writer( reply);

                     serviceFound->second->lookedup++;
                     log::internal::debug << "serviceFound->second->lookedup: " << serviceFound->second->lookedup << std::endl;
                  }
                  else
                  {
                     //
                     // All servers are busy, we stack the request
                     //
                     m_state.pending.push_back( std::move( message));
                  }
               }
               else
               {
                  //
                  // TODO: We will send the request to the gateway.
                  //
                  // Server (queue) that hosts the requested service is not found.
                  // We propagate this by having 0 occurrence of server in the response
                  //
                  message::service::name::lookup::Reply reply;
                  reply.service.name = message.requested;

                  queue_writer_type writer( message.server.queue_id, m_state);
                  writer( reply);

               }

            }
         };

         typedef basic_servicelookup< QueueBlockingWriter> ServiceLookup;


         //!
         //! Handles ACK from services.
         //!
         //! if there are pending request for the "acked-service" we
         //! send response directly
         //!
         template< typename Q>
         struct basic_ack : public Base
         {
            typedef Q queue_writer_type;
            typedef basic_servicelookup< Q> servicelookup_type;

            typedef message::service::ACK message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               //
               // find instance and flag it as idle
               //
               auto instance = m_state.instances.find( message.server.pid);

               if( instance != std::end( m_state.instances))
               {
                  instance->second->alterState( Server::Instance::State::idle);
                  instance->second->invoked++;

                  if( ! m_state.pending.empty())
                  {
                     //
                     // There are pending requests, check if there is one that is
                     // waiting for a service that this, now idle, instance has advertised.
                     //
                     auto pendingFound = common::range::find_if(
                        common::range::make( m_state.pending),
                        action::find::Pending( instance->second));

                     if( ! pendingFound.empty())
                     {
                        //
                        // We now know that there are one idle server that has advertised the
                        // requested service (we've just marked it as idle...).
                        // We can use the normal request to get the response
                        //
                        servicelookup_type serviceLookup( m_state);
                        serviceLookup.dispatch( *pendingFound.first);

                        //
                        // Remove pending
                        //
                        m_state.pending.erase( pendingFound.first);
                     }
                  }
               }
               else
               {
                  // TODO: log this? This can only happen if there are some logic error or
                  // the system is in a inconsistent state.
               }
            }
         };


         typedef basic_ack< QueueBlockingWriter> ACK;


         //!
         //! Broker needs to have it's own policy for callee::handle::basic_call, since
         //! we can't communicate with blocking to the same queue (with read, who is
         //! going to write? with write, what if the queue is full?)
         //!
         struct Policy
         {

            typedef QueueBlockingWriter reply_writer;
            typedef QueueNonBlockingWriter monitor_writer;

            Policy( broker::State& state) : m_state( state) {}

            Policy( Policy&&) = default;
            Policy& operator = ( Policy&&) = default;


            void connect(  message::server::connect::Request& message, const std::vector< common::transaction::Resource>& resources)
            {

               message.server.queue_id = ipc::receive::id();
               message.path = common::process::path();

               //
               // We add the server
               //
               auto instance = m_state.instances.emplace(
                     message.server.pid, action::transform::Instance()( message)).first;
               //
               // Add services
               //
               common::range::for_each(
                  common::range::make( message.services),
                  action::add::Service( m_state, instance->second));

               //
               // Set our instance in 'ready' mode
               //
               action::connect( instance->second, message);

            }

            void disconnect()
            {
               message::server::Disconnect message;

               Disconnect disconnect( m_state);
               disconnect.dispatch( message);
            }

            void reply( platform::queue_id_type id, message::service::Reply& message)
            {
               reply_writer writer( id, m_state);
               writer( message);
            }

            void ack( const message::service::callee::Call& message)
            {
               message::service::ACK ack;
               ack.server.queue_id = ipc::getReceiveQueue().id();
               ack.service = message.service.name;

               ACK sendACK( m_state);
               sendACK.dispatch( ack);
            }

            void transaction( const message::service::callee::Call&, const server::Service&)
            {

            }

            void transaction( const message::service::Reply& message)
            {

            }

            void statistics( platform::queue_id_type id, message::monitor::Notify& message)
            {
               //
               // We don't collect statistics for the broker
               //
            }


         private:

            broker::State& m_state;

         };

         typedef common::callee::handle::basic_call< broker::handle::Policy> Call;


		} // handle

	} // broker
} // casual


#endif /* CASUAL_BROKER_TRANSFORM_H_ */
