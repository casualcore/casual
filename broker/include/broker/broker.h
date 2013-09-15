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
#include "common/environment.h"


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
	         Resource() = default;
	         Resource( Resource&&) = default;

	         Resource( const std::string& key, const std::string& openinfo, const std::string& closeinfo)
	         : key( key), openinfo( openinfo), closeinfo( closeinfo) {}

	         std::size_t id = nextId();
	         std::string key;
            std::string openinfo;
            std::string closeinfo;

            static std::size_t nextId()
            {
               static std::size_t id = 0;
               return ++id;
            }

	      };


	      std::string name;
	      std::string note;

	      std::vector< Resource> resource;
         std::vector< std::shared_ptr< Group>> dependencies;


      };

	   inline bool operator == ( const std::shared_ptr< Group>& lhs, const std::shared_ptr< Group>& rhs) { return lhs->name == rhs->name;}


	   struct Service;

		struct Server
		{
		   typedef common::message::server::Id::pid_type pid_type;

		   struct Instance
		   {

            enum class State
            {
               absent,
               prospect,
               idle,
               busy,
               shutdown
            };

            void alterState( State state)
            {
               this->state = state;
               last = common::clock_type::now();
            }


            pid_type pid = 0;
            common::message::server::Id::queue_id_type queue_id = 0;
            std::shared_ptr< Server> server;
            std::vector< std::shared_ptr< Service>> services;
            std::size_t invoked = 0;
            common::time_type last;
		   //private:
            State state = State::absent;
		   };


		   std::string alias;
			std::string path;
			std::string note;
			std::vector< std::string> arguments;

			std::vector< std::shared_ptr< Group>> memberships;

			std::vector< std::shared_ptr< Server::Instance>> instances;

			std::vector< std::string> restrictions;

			bool restart = false;

		};


		struct Service
		{
			Service( const std::string& name) : information( name) {}

			Service() {}

			common::message::Service information;
			std::size_t lookedup = 0;
			std::vector< std::shared_ptr< Server::Instance>> instances;
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
		   State() = default;

		   State( State&&) = default;
		   State( const State&) = delete;


		   typedef std::unordered_map< std::string, std::shared_ptr< Server>> server_mapping_type;
		   typedef std::unordered_map< Server::pid_type, std::shared_ptr< Server::Instance>> instance_mapping_type;
		   typedef std::unordered_map< std::string, std::shared_ptr< Service>> service_mapping_type;
		   typedef std::deque< common::message::service::name::lookup::Request> pending_requests_type;

		   typedef std::map< std::string, std::shared_ptr< Group>> group_mapping_type;

		   server_mapping_type servers;
		   instance_mapping_type instances;
		   service_mapping_type services;
		   group_mapping_type groups;

		   pending_requests_type pending;

		   //std::vector< common::platform::pid_type> processes;

		   // TODO: Temp
		   common::platform::queue_id_type monitorQueue = 0;

		   std::shared_ptr< Server> transactionManager;

		   common::platform::queue_id_type transactionManagerQueue = 0;

		};

		struct Settings
		{

		   std::string configurationfile = common::environment::file::configuration();

		};

		class Broker
		{
		public:

		   static Broker& instance();
			~Broker();

			void start( const Settings& arguments);


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
