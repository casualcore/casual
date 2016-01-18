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

#include "common/communication/ipc.h"
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



			const State& state() const
			{
			   return m_state;
			}


		private:

			void terminate();

			common::file::scoped::Path m_brokerQueueFile;
			State m_state;

		};

		namespace update
      {
         void instances( State& state, const std::vector< admin::update::InstancesVO>& instances);

      } // update

      admin::ShutdownVO shutdown( State& state, bool broker);


		namespace message
      {
         void pump( State& state);

      } // message

	} // broker
} // casual




#endif /* CASUAL_BROKER_H_ */
