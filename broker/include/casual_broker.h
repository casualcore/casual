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

			utility::platform::pid_type m_pid;
			std::string m_path;
			ipc::send::Queue::queue_key_type m_queue_key;
		};

		struct ServerHolder
		{

		private:
			std::set< Server> m_servers;
		};

		struct Service
		{
			Service( const Server& server) : m_requested( 0), m_server( server) {}

			std::string m_name;
			std::size_t m_requested;
			const Server& m_server;
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

			std::list< Server> m_servers;
			std::map< std::string, Service> m_services;


		};
	}

}




#endif /* CASUAL_BROKER_H_ */
