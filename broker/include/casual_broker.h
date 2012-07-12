//!
//! casual_broker.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_H_
#define CASUAL_BROKER_H_

#include <vector>
#include <string>
#include <map>
#include <list>
#include <deque>

#include "casual_ipc.h"
#include "casual_utility_file.h"
#include "casual_message.h"
#include "casual_utility_platform.h"

namespace casual
{
	namespace broker
	{

		struct Server
		{

		   typedef message::ServerId::pid_type pid_type;

			Server() : pid( 0), queue_key( 0), idle( true) {}

			pid_type pid;
			std::string path;
			message::ServerId::queue_key_type queue_key;
			bool idle;
			//std::set< std::string> services;


			bool operator < ( const Server& rhs) const
			{
			   return pid < rhs.pid;
			}
		};

		typedef std::map< Server::pid_type, Server> server_mapping_type;



		struct Service
		{
			Service( const std::string& name) : name( name) {}

			Service() {}

			void add( Server* server)
			{
				servers.push_back( server);
				m_currentServer = servers.begin();
			}

			const Server& nextServer()
			{
				if( m_currentServer == servers.end())
				{
					m_currentServer = servers.begin();
				}

				return **m_currentServer++;
			}

			std::string name;
			std::vector< Server*> servers;

		private:

			std::vector< Server*>::iterator m_currentServer;
		};

		struct State
		{
		   typedef std::map< std::string, Service> service_mapping_type;
		   typedef std::deque< message::ServiceRequest> pending_requests_type;

		   server_mapping_type servers;
		   service_mapping_type services;
		   pending_requests_type pending;
		};

		class Broker
		{
		public:

			Broker( const std::vector< std::string>& arguments);
			~Broker();

			void start();

		private:
			utility::file::ScopedPath m_brokerQueueFile;
			ipc::receive::Queue m_receiveQueue;

			State m_state;

		};
	}

}




#endif /* CASUAL_BROKER_H_ */
