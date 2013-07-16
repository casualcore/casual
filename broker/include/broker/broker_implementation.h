//!
//! casual_broker_transform.h
//!
//! Created on: Jun 15, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_TRANSFORM_H_
#define CASUAL_BROKER_TRANSFORM_H_

#include "broker/broker.h"

#include "common/message.h"
#include "common/queue.h"
#include "common/logger.h"
#include "common/environment.h"
#include "common/server_context.h"

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
		namespace transform
		{
		   struct Service
		   {
		      std::string operator() ( const message::Service& message)
		      {
		         return message.name;
		      }
		   };

			struct Server
			{

				casual::broker::Server operator () ( const message::server::Connect& message) const
				{
					casual::broker::Server result;

					result.path = message.path;
					result.pid = message.serverId.pid;
					result.queue_key = message.serverId.queue_key;

					return result;
				}


				message::server::Id operator () ( const casual::broker::Server& value) const
				{
					casual::message::server::Id result;

					result.pid = value.pid;
					result.queue_key = value.queue_key;

					return result;
				}

			};


		} // transform

		namespace find
		{
		   struct Server
		   {
		      Server( broker::Server::pid_type pid) : m_pid( pid) {}

		      bool operator () ( const broker::Server* server)
            {
		         return server->pid == m_pid;
            }

		      bool operator () ( const State::service_mapping_type::value_type & service) const
            {
               return std::find_if(
                     std::begin( service.second.servers),
                     std::end( service.second.servers),
                     find::Server( m_pid)) != std::end( service.second.servers);
            }

		   private:
		      broker::Server::pid_type m_pid;
		   };

		   namespace server
		   {
		      struct Idle
		      {
		         bool operator () ( const broker::Server* server)
               {
                  return server->idle == true;
               }
		      };

		   } // server

		   struct Pending
		   {
		      Pending( const std::vector< std::string>& services) : m_services( services) {}

		      bool operator () ( const message::service::name::lookup::Request& request)
		      {
		         return std::binary_search( std::begin( m_services), std::end( m_services), request.requested);
		      }

		      std::vector< std::string> m_services;
		   };

		} // find

		namespace extract
		{

		   //!
		   //! Extract the services associated with the specific server
		   //!
		   //! @return a sorted range with the services.
		   //!
		   std::vector< std::string> services( broker::Server::pid_type pid, State& state)
         {
		      std::vector< std::string> result;

		      auto current = state.services.begin();

		      while( ( current = std::find_if(
		            current,
		            state.services.end(),
		            find::Server( pid))) != state.services.end())
		      {
		         result.push_back( current->first);
		         ++current;
		      }

		      std::sort( std::begin( result), std::end( result));

		      return result;
         }

		}

		namespace state
		{
		   namespace internal
		   {

		      struct Service
            {
               Service( broker::Server* server, State::service_mapping_type& serviceMapping)
                  : m_server( server), m_serviceMapping( serviceMapping) {}

               void operator() ( const message::Service& service)
               {
                  //
                  // If the service is not registered before, it will be added now... Otherwise
                  // we use the current one...
                  //

                  auto findIter = m_serviceMapping.emplace(
                        service.name, broker::Service{ service.name}).first;

                  //
                  // Add the pointer to the server
                  //
                  findIter->second.add( m_server);
               }

            private:
               broker::Server* m_server;
               State::service_mapping_type& m_serviceMapping;
            };


            void removeService( const std::string& name, broker::Server::pid_type pid, State& state)
            {
               auto findIter = state.services.find( name);

               if( findIter != state.services.end())
               {
                  //
                  // We found the service. Now we remove the corresponding server
                  //

                  typedef std::vector< broker::Server*> servers_type;

                  auto serversEnd = std::remove_if(
                        std::begin( findIter->second.servers),
                        std::end( findIter->second.servers),
                        find::Server( pid));

                  findIter->second.servers.erase( serversEnd, std::end( findIter->second.servers));

                  //
                  // If servers is empty we remove the service all together.
                  //
                  if( findIter->second.servers.empty())
                  {
                     state.services.erase( findIter);
                  }
               }



            }

            template< typename Iter>
            void removeServices( Iter start, Iter end, broker::Server::pid_type pid, State& state)
            {
               for( ; start != end; ++start)
               {
                  removeService( start->name, pid, state);
               }
            }

		   }

		} // state



		namespace handle
		{
		   struct Base
		   {
		   protected:
		      Base( State& state) : m_state( state) {}

		      State& m_state;
		   };

         //!
         //! Monitor Connect
         //!
         struct MonitorConnect: public Base
         {
            typedef message::monitor::Connect message_type;

            MonitorConnect( State& state)
                  : Base( state)
            {
            }

            void dispatch( message_type& message)
            {
               //TODO: Temp
               m_state.monitorQueue = message.serverId.queue_key;
            }
         };

         //!
         //! Monitor Disconnect
         //!
         struct MonitorDisconnect: public Base
         {
            typedef message::monitor::Disconnect message_type;

            MonitorDisconnect( State& state)
                  : Base( state)
            {
            }

            void dispatch( message_type& message)
            {
               m_state.monitorQueue = 0;
            }

         };

         //!
         //! Transaction Manager Connect
         //!
         struct TransactionManagerConnect: public Base
         {
            typedef message::transaction::Connect message_type;

            TransactionManagerConnect( State& state)
                  : Base( state)
            {
            }

            void dispatch( message_type& message)
            {
               m_state.transactionManagerQueue = message.serverId.queue_key;
            }
         };

         //!
         //! Advertise 0..N services for a server.
         //!
         struct Advertise : public Base
         {
            typedef message::service::Advertise message_type;

            Advertise( State& state) : Base( state) {}

            template< typename M>
            void dispatch( M& message)
            {
               //
               // Find the server
               //
               auto find = m_state.servers.find( message.serverId.pid);

               if( find != std::end( m_state.servers))
               {
                  //
                  // Add all the services, with this corresponding server
                  //
                  std::for_each(
                     std::begin( message.services),
                     std::end( message.services),
                     state::internal::Service( &find->second, m_state.services));

               }
               else
               {

                  logger::error << "server (pid: " << message.serverId.pid << ") has not connected before advertising services";

               }
            }
         };

         //!
         //! A server is disconnected
         //!
         struct Disconnect : public Base
         {
            typedef message::server::Disconnect message_type;

            Disconnect( State& state) : Base( state) {}

            template< typename M>
            void dispatch( M& message)
            {
               //
               // Find all corresponding services this server have advertise
               //

               auto start = std::begin( m_state.services);

               while( ( start = std::find_if(
                     start,
                     std::end( m_state.services),
                     find::Server( message.serverId.pid))) != std::end( m_state.services))
               {
                  //
                  // remove the service
                  //
                  state::internal::removeService( start->first, message.serverId.pid, m_state);
               }

               //
               // Remove the server
               //
               m_state.servers.erase( message.serverId.pid);
            }
         };

         template< typename Q>
         struct basic_connect : public Base
         {
            typedef message::server::Connect message_type;
            typedef Q queue_writer_type;

            basic_connect( State& state) : Base( state) {}

            void dispatch( message_type& message)
            {

               auto find =  m_state.servers.find( message.serverId.pid);

               if( find != std::end( m_state.servers))
               {
                  //
                  // We got the server... Why?
                  // - log
                  // - remove
                  //
                  logger::error << "server (pid: " << message.serverId.pid << ") is already connected";

                  Disconnect disconnect( m_state);
                  disconnect.dispatch( message);
               }

               //
               // We add the server
               //
               m_state.servers.emplace(
                     message.serverId.pid, transform::Server()( message));

               //
               // Server is started for the first time.
               // Send some configuration
               //
               message::server::Configuration configuation;
               configuation.transactionManagerQueue = m_state.transactionManagerQueue;

               queue_writer_type writer( message.serverId.queue_key);
               writer( configuation);


               //
               // Advertiese the servies
               //
               Advertise advertise( m_state);
               advertise.dispatch( message);
            }
         };

         typedef basic_connect< queue::basic_queue< ipc::send::Queue, queue::blocking::Writer>> Connect;


		   //!
         //! Unadvertise 0..N services for a server.
         //!
		   struct Unadvertise : public Base
         {
            typedef message::service::Unadvertise message_type;

            Unadvertise( State& state) : Base( state) {}

            void dispatch( message_type& message)
            {
               state::internal::removeServices(
                  std::begin( message.services),
                  std::end( message.services),
                  message.serverId.pid,
                  m_state);

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

            basic_servicelookup( State& state) : Base( state) {}

            void dispatch( message_type& message)
            {
               auto serviceFound = m_state.services.find( message.requested);

               if( serviceFound != std::end( m_state.services) && ! m_state.servers.empty())
               {
                  //
                  // Try to find an idle server.
                  //
                  auto idleServer = std::find_if(
                        std::begin( serviceFound->second.servers),
                        std::end( serviceFound->second.servers),
                        find::server::Idle());

                  if( idleServer != std::end( serviceFound->second.servers))
                  {
                     //
                     // flag it as not idle.
                     //
                     (*idleServer)->idle = false;

                     message::service::name::lookup::Reply reply;
                     reply.service = serviceFound->second.information;
                     reply.service.monitor_queue = m_state.monitorQueue;
                     reply.server.push_back( transform::Server()( **idleServer));


                     queue_writer_type writer( message.server.queue_key);

                     writer( reply);
                  }
                  else
                  {
                     //
                     // All servers are busy, we stack the request
                     //
                     m_state.pending.push_back( message);
                  }
               }
               else
               {
                  //
                  // Server (queue) that hosts the requested service is not found.
                  // We propagate this by having 0 occurrence of server in the response
                  //
                  message::service::name::lookup::Reply reply;
                  reply.service.name = message.requested;

                  queue_writer_type writer( message.server.queue_key);
                  writer( reply);

               }

            }
         };

         typedef basic_servicelookup< queue::basic_queue< ipc::send::Queue, queue::blocking::Writer>> ServiceLookup;


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

            basic_ack( State& state) : Base( state) {}

            void dispatch( message_type& message)
            {
               //
               // find server and flag it as idle
               //
               auto findIter = m_state.servers.find( message.server.pid);

               if( findIter != std::end( m_state.servers))
               {
                  findIter->second.idle = true;

                  if( ! m_state.pending.empty())
                  {
                     //
                     // There are pending requests, check if there is one that is
                     // waiting for a service that this, now idle, server has advertised.
                     //
                     auto pendingIter = std::find_if(
                           std::begin( m_state.pending),
                           std::end( m_state.pending),
                           find::Pending( extract::services( message.server.pid, m_state)));

                     if( pendingIter != std::end( m_state.pending))
                     {
                        //
                        // We now know that there are one idle server that has advertised the
                        // requested service (we just marked it as idle...).
                        // We can use the normal request to get the response
                        //
                        servicelookup_type serviceLookup( m_state);
                        serviceLookup.dispatch( *pendingIter);

                        //
                        // Remove pending
                        //
                        m_state.pending.erase( pendingIter);
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


         typedef basic_ack< queue::basic_queue< ipc::send::Queue, queue::blocking::Writer>> ACK;


         //!
         //! Broker needs to have it's own policy for callee::handle::basic_call, since
         //! we can't communicate with blocking to the same queue (with read, who is
         //! going to write? with write, what if the queue is full?)
         //!
         struct Policy
         {

            typedef queue::basic_queue< ipc::send::Queue, queue::blocking::Writer> reply_writer;

            Policy( broker::State& state) : m_state( state) {}


            void connect( message::server::Connect& message)
            {

               message.serverId.queue_key = ipc::getReceiveQueue().getKey();
               message.path = common::environment::getExecutablePath();

               //
               // We add the server
               //
               m_state.servers.emplace(
                     message.serverId.pid, transform::Server()( message));
               //
               // Advertiese the servies
               //
               Advertise advertise( m_state);
               advertise.dispatch( message);
            }

            void disconnect()
            {
               message::server::Disconnect message;

               Disconnect disconnect( m_state);
               disconnect.dispatch( message);
            }


            void ack( const message::service::callee::Call& message)
            {
               message::service::ACK ack;
               ack.server.queue_key = ipc::getReceiveQueue().getKey();
               ack.service = message.service.name;

               ACK sendACK( m_state);
               sendACK.dispatch( ack);
            }


         private:

            broker::State& m_state;

         };

         typedef common::callee::handle::basic_call< broker::handle::Policy> Call;


		} // handle

	} // broker
} // casual


#endif /* CASUAL_BROKER_TRANSFORM_H_ */
