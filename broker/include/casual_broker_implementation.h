//!
//! casual_broker_transform.h
//!
//! Created on: Jun 15, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_TRANSFORM_H_
#define CASUAL_BROKER_TRANSFORM_H_

#include "casual_broker.h"

#include "casual_message.h"

//
// std
//
#include <algorithm>
#include <vector>
#include <string>


namespace casual
{

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

				casual::broker::Server operator () ( const message::ServiceAdvertise& message) const
				{
					casual::broker::Server result;

					result.path = message.serverPath;
					result.pid = message.serverId.pid;
					result.queue_key = message.serverId.queue_key;

					return result;
				}


				casual::message::ServerId operator () ( const casual::broker::Server& value) const
				{
					casual::message::ServerId result;

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
		      Server( message::ServerId::pid_type pid) : m_pid( pid) {}

		      bool operator () ( const broker::Server* server)
            {
		         return server->pid == m_pid;
            }

		      bool operator () ( const State::service_mapping_type::value_type & service) const
            {
               return std::find_if(
                     service.second.servers.begin(),
                     service.second.servers.end(),
                     find::Server( m_pid)) != service.second.servers.end();
            }

		   private:
		      message::ServerId::pid_type m_pid;
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

		      bool operator () ( const message::ServiceRequest& request)
		      {
		         return std::binary_search( m_services.begin(), m_services.end(), request.requested);
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
		   std::vector< std::string> services( message::ServerId::pid_type pid, State& state)
         {
		      std::vector< std::string> result;

		      State::service_mapping_type::iterator current = state.services.begin();

		      while( ( current = std::find_if(
		            current,
		            state.services.end(),
		            find::Server( pid))) != state.services.end())
		      {
		         result.push_back( current->first);
		         ++current;
		      }

		      std::sort( result.begin(), result.end());

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

                  State::service_mapping_type::iterator findIter = m_serviceMapping.insert(
                        std::make_pair( service.name, broker::Service( service.name))).first;

                  //
                  // Add the pointer to the server
                  //
                  findIter->second.add( m_server);
               }

            private:
               broker::Server* m_server;
               State::service_mapping_type& m_serviceMapping;
            };

            void removeService( const std::string& name, message::ServerId::pid_type pid, State& state)
            {
               State::service_mapping_type::iterator findIter = state.services.find( name);

               if( findIter != state.services.end())
               {
                  //
                  // We found the service. Now we remove the corresponding server
                  //

                  typedef std::vector< broker::Server*> servers_type;

                  servers_type::iterator serversEnd = std::remove_if(
                        findIter->second.servers.begin(),
                        findIter->second.servers.end(),
                        find::Server( pid));

                  findIter->second.servers.erase( serversEnd, findIter->second.servers.end());

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
            void removeServices( Iter start, Iter end, message::ServerId::pid_type pid, State& state)
            {
               for( ; start != end; ++start)
               {
                  removeService( start->name, pid, state);
               }
            }
		   }

		   //!
         //! Unadvertise 0..N services for a server.
         //!
		   void unadvertiseService( const message::ServiceUnadvertise& message, State& state)
		   {
		      internal::removeServices(
		            message.services.begin(),
		            message.services.end(),
		            message.serverId.pid,
		            state);

		   }


		   //!
         //! Advertise 0..N services for a server.
         //!
		   void advertiseService( const message::ServiceAdvertise& message, State& state)
		   {

            //
            // If the server is not registered before, it will be added now... Otherwise
		      // we use the current one...
            //
            server_mapping_type::iterator serverIterator = state.servers.insert(
                  std::make_pair( message.serverId.pid, transform::Server()( message))).first;

            //
            // Add all the services, with this corresponding server
            //
            std::for_each(
               message.services.begin(),
               message.services.end(),
               internal::Service( &serverIterator->second, state.services));

		   }

		   //!
		   //! Removes a server, and its advertised services from the broker
		   //!
		   void removeServer( const message::ServerDisconnect& message, State& state)
         {

		      //
		      // Find all corresponding services this server have advertise
		      //

		      State::service_mapping_type::iterator start = state.services.begin();

		      while( ( start = std::find_if(
		            start,
		            state.services.end(),
		            find::Server( message.serverId.pid))) != state.services.end())
		      {
		         //
		         // remove the service
		         //
		         internal::removeService( start->first, message.serverId.pid, state);
		      }

		      //
		      // Remove the server
		      //
		      state.servers.erase( message.serverId.pid);
         }

		   //!
         //! Tries to find the corresponding server to the service requested.
		   //!
		   //! @return 0..1 message::ServiceResponse
		   //! - 0 occurrence, No idle servers, request is stacked to pending. No response is sent.
		   //! - 1 occurrence, idle server found, or service not found at all (ie TPENOENT).
		   //!      either way, response is sent to requested queue.
         //!
		   std::vector< message::ServiceResponse> requestService( const message::ServiceRequest& message, State& state)
		   {
		      std::vector< message::ServiceResponse> result;

		      State::service_mapping_type::iterator serviceFound = state.services.find( message.requested);

		      if( serviceFound != state.services.end() && !state.servers.empty())
		      {
		         //
		         // Try to find an idle server.
		         //
		         std::vector< Server*>::iterator idleServer = std::find_if(
		               serviceFound->second.servers.begin(),
		               serviceFound->second.servers.end(),
		               find::server::Idle());

		         if( idleServer != serviceFound->second.servers.end())
		         {
		            //
		            // flag it as not idle.
		            //
		            (*idleServer)->idle = false;

		            message::ServiceResponse response;
		            response.service = serviceFound->second.information;
		            response.server.push_back( transform::Server()( **idleServer));

		            result.push_back( response);
		         }
		         else
		         {
		            //
		            // All servers are busy, we stack the request
		            //
		            state.pending.push_back( message);
		         }
		      }
		      else
		      {
		         //
		         // Server (queue) that hosts the requested service is not found.
		         // We propagate this by having 0 occurrence of server in the response
		         //
		         message::ServiceResponse response;
		         response.service.name = message.requested;
		         result.push_back( response);

		      }
		      return result;
		   }

		   typedef std::pair< message::ServerId::queue_key_type, message::ServiceResponse> PendingResponse;

		   //!
		   //! When a service is done.
		   //! - Mark the corresponding server as idle
		   //! - Check if there are any pending requests
		   //! - if so, mark the server as busy again, and return the response
		   //!
		   //! @return 0..1 pending responses.
		   //!
		   std::vector< PendingResponse> serviceDone( message::ServiceACK& message, State& state)
		   {
		      std::vector< PendingResponse> result;

		      //
		      // find server and flag it as idle
		      //
		      server_mapping_type::iterator findIter = state.servers.find( message.server.pid);

		      if( findIter != state.servers.end())
		      {
		         findIter->second.idle = true;

		         if( !state.pending.empty())
		         {
		            //
		            // There are pending requests, check if there is one that is
		            // waiting for a service that this, now idle, server has advertised.
		            //
		            State::pending_requests_type::iterator pendingIter = std::find_if(
		                  state.pending.begin(),
		                  state.pending.end(),
		                  find::Pending( extract::services( message.server.pid, state)));

		            if( pendingIter != state.pending.end())
		            {
		               //
		               // We now know that there are one idle server that has advertised the
		               // requested service (we just marked it as idle...).
		               // We can use the normal request to get the response
		               //

		               std::vector< message::ServiceResponse> response = requestService( *pendingIter, state);

		               if( response.empty())
		               {
		                  //
		                  // Something is very wrong!
		                  //
		                  state.pending.pop_back();
		                  throw exception::NotReallySureWhatToNameThisException();
		               }

		               result.push_back( std::make_pair( pendingIter->server.queue_key, response.front()));

		               //
		               // The server is busy again... No rest for the wicked...
		               //
		               findIter->second.idle = false;

		               //
		               // Remove pending
		               //
		               state.pending.erase( pendingIter);
		            }
		         }
		      }
		      else
		      {
		         // TODO: log this? This can only happen if there are some logic error or
		         // the system is in a inconsistent state.
		      }

		      return result;
		   }

		} // state


	} // broker
} // casual


#endif /* CASUAL_BROKER_TRANSFORM_H_ */
