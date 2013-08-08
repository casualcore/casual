//!
//! casual_broker.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_H_
#define CASUAL_BROKER_H_




#include "common/file.h"
#include "common/platform.h"

#include "common/ipc.h"
#include "common/message.h"


#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <list>
#include <deque>


namespace casual
{
	namespace broker
	{



	   struct Group
      {
	      struct Resource
	      {
	         std::string key;
            std::string openinfo;
            std::string closeinfo;
	      };


	      std::string name;
	      std::string note;

	      std::vector< Resource> resource;
         std::vector< std::shared_ptr< Group>> dependencies;


      };

	   inline bool operator == ( const std::shared_ptr< Group>& lhs, const std::shared_ptr< Group>& rhs) { return lhs->name == rhs->name;}




		struct Server
		{
		   typedef common::message::server::Id::pid_type pid_type;

		   //struct Instance
		   //{
            enum class State
            {
               absent,
               prospect,
               idle,
               busy,
               shutdown
            };

            pid_type pid = 0;
            common::message::server::Id::queue_id_type queue_id = 0;
            State state = State::absent;

            //std::shared_ptr< Server> server;

            /*
            bool operator < ( const Instance& rhs) const
            {
               return pid < rhs.pid;
            }
            */
		   //};


		   std::string alias;
			std::string path;
			std::string note;

			std::vector< std::shared_ptr< Group>> memberships;

			//std::vector< Instance> instances;


		};




		struct Service
		{
			Service( const std::string& name) : information( name) {}

			Service() {}

			/*
			void add( Server* server)
			{
				servers.push_back( server);
			}
			*/

			common::message::Service information;
			std::vector< std::shared_ptr< Server>> servers;
		};


		struct Executable
      {
         typedef common::message::server::Id::pid_type pid_type;

         std::string alias;
         std::string path;
         std::string arguments;
         std::string note;

         std::vector< pid_type> instances;

         std::vector< std::shared_ptr< Group>> memberships;
      };


		struct State
		{
		   typedef std::unordered_map< Server::pid_type, std::shared_ptr< Server>> server_mapping_type;
		   typedef std::unordered_map< std::string, Service> service_mapping_type;
		   typedef std::deque< common::message::service::name::lookup::Request> pending_requests_type;

		   typedef std::map< std::string, std::shared_ptr< Group>> group_mapping_type;

		   server_mapping_type servers;
		   service_mapping_type services;
		   pending_requests_type pending;
		   group_mapping_type groups;

		   std::vector< common::platform::pid_type> processes;

		   // TODO: Temp
		   common::platform::queue_id_type monitorQueue = 0;

		   common::platform::queue_id_type transactionManagerQueue = 0;

		};

		class Broker
		{
		public:

		   static Broker& instance();
			~Broker();

			void start( const std::vector< std::string>& arguments);


			//void addServers( const std::vector< action::server::>)

			//void bootServers( const std::vector< action::server::Instances>& servers);

			const State& state() const
			{
			   return m_state;
			}


		private:
			Broker();

			common::file::ScopedPath m_brokerQueueFile;
			common::ipc::receive::Queue& m_receiveQueue = common::ipc::getReceiveQueue();

			State m_state;

		};
	}

}




#endif /* CASUAL_BROKER_H_ */
