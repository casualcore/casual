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
#include <unordered_map>
#include <list>
#include <deque>

#include "broker/configuration.h"

#include "utility/file.h"
#include "utility/platform.h"

#include "common/ipc.h"
#include "common/message.h"


namespace casual
{
	namespace broker
	{



		struct Server
		{

		   typedef message::server::Id::pid_type pid_type;

			Server() : pid( 0), queue_key( 0), idle( true) {}

			pid_type pid;
			std::string path;
			message::server::Id::queue_key_type queue_key;
			bool idle;


			bool operator < ( const Server& rhs) const
			{
			   return pid < rhs.pid;
			}
		};




		struct Service
		{
			Service( const std::string& name) : information( name) {}

			Service() {}

			void add( Server* server)
			{
				servers.push_back( server);
			}

			message::Service information;
			std::vector< Server*> servers;
		};

		struct State
		{
		   typedef std::unordered_map< Server::pid_type, Server> server_mapping_type;
		   typedef std::unordered_map< std::string, Service> service_mapping_type;
		   typedef std::deque< message::service::name::lookup::Request> pending_requests_type;

		   server_mapping_type servers;
		   service_mapping_type services;
		   pending_requests_type pending;

		   configuration::Settings configuration;
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
