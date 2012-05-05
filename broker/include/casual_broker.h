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

namespace casual
{
	namespace broker
	{

		struct Server
		{

			long m_pid;
			std::string m_path;
			ipc::send::Queue m_queue;
		};

		struct Service
		{

			std::size_t m_requested;
			Server& server;
		};

		class Broker
		{
		public:
			Broker( const std::vector< std::string>& arguments);
			~Broker();

			void start();

		private:
			ipc::receive::Queue m_receiveQueue;

			std::list< Server> m_servers;
			std::map< std::string, Service> m_services;


		};
	}

}




#endif /* CASUAL_BROKER_H_ */
