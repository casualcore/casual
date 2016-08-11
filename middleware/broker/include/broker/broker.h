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
		   std::string forward;
		};

		class Broker
		{
		public:

		   Broker( Settings&& settings);
			~Broker();

			void start();



			const State& state() const
			{
			   return m_state;
			}


		private:

			State m_state;

		};


		namespace message
      {
         void pump( State& state);

      } // message

	} // broker
} // casual




#endif /* CASUAL_BROKER_H_ */
