//!
//! casual_broker.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_H_
#define CASUAL_BROKER_H_


#include "broker/state.h"


#include "common/file.h"
#include "common/platform.h"

#include "common/ipc.h"
#include "common/message/server.h"
#include "common/environment.h"

#include "broker/admin/brokervo.h"






namespace casual
{
   namespace broker
   {

		struct Settings
		{

		   std::string configurationfile = common::environment::file::configuration();

		};

		class Broker
		{
		public:

		   Broker( const Settings& arguments);
			~Broker();

			void start();


			//void addServers( const std::vector< action::server::>)

			void serverInstances( const std::vector< admin::update::InstancesVO>& instances);


			admin::ShutdownVO shutdown();


			const State& state() const
			{
			   return m_state;
			}


		private:


			void terminate();


			common::file::scoped::Path m_brokerQueueFile;
			common::ipc::receive::Queue& m_receiveQueue = common::ipc::receive::queue();

			State m_state;

		};
	} // broker
} // casual




#endif /* CASUAL_BROKER_H_ */
