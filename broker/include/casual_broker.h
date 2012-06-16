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
#include <set>

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

			Server() : m_requested( 0) {}

			utility::platform::pid_type m_pid;
			std::string m_path;
			std::size_t m_requested;
			ipc::send::Queue::queue_key_type m_queue_key;
		};

		typedef std::list< Server> Servers;



		struct Service
		{
			Service( const std::string& name) : m_name( name), m_requested( 0) {}

			void add( Servers::iterator server)
			{
				m_servers.push_back( server);
				m_currentServer = m_servers.begin();
			}

			Server& nextServer()
			{
				if( m_currentServer == m_servers.end())
				{
					m_currentServer = m_servers.begin();
				}
				Servers::iterator result = *m_currentServer++;
				return *result;
			}

		private:
			std::string m_name;
			std::size_t m_requested;
			std::vector< Servers::iterator> m_servers;
			std::vector< Servers::iterator>::iterator m_currentServer;
		};


		class Broker
		{
		public:

			typedef std::map< std::string, Service> service_mapping_type;

			Broker( const std::vector< std::string>& arguments);
			~Broker();

			void start();

		private:
			utility::file::ScopedPath m_brokerQueueFile;
			ipc::receive::Queue m_receiveQueue;

			Servers m_servers;
			service_mapping_type m_services;


		};
	}

}




#endif /* CASUAL_BROKER_H_ */
