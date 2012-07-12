//!
//! casual_broker_transform.h
//!
//! Created on: Jun 15, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_TRANSFORM_H_
#define CASUAL_BROKER_TRANSFORM_H_

#include "casual_broker.h"


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

		      bool operator () ( const std::pair< std::string, broker::Service>& service) const
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

		} // find

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

		   void unadvertiseService( const message::ServiceUnadvertise& message, State& state)
		   {
		      internal::removeServices(
		            message.services.begin(),
		            message.services.end(),
		            message.serverId.pid,
		            state);

		   }


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

		   std::vector< message::ServiceResponse> requestService( const message::ServiceRequest& message, State& state)
		   {
		      std::vector< message::ServiceResponse> result;

		      State::service_mapping_type::iterator serviceFound = state.services.find( message.requested);

		      if( serviceFound != state.services.end())
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
		            (*idleServer)->idle;

		            message::ServiceResponse response;
		            response.requested = message.requested;
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
		         // Service not found. We propagate this by having 0 presence of server in the response
		         //
		         message::ServiceResponse response;
		         response.requested = message.requested;
		         result.push_back( response);

		      }
		      return result;
		   }

		   void serviceDone( message::ServiceACK& message, State& state)
		   {
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
		            // There are pendning requests
		            //

		         }
		      }
		      else
		      {
		         // TODO: log this?
		      }
		   }

		}


	} // broker
} // casual


#endif /* CASUAL_BROKER_TRANSFORM_H_ */
