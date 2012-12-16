/*
 * casual_statisticshandler.h
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#ifndef CASUAL_MONITOR_H_
#define CASUAL_MONITOR_H_

#include <vector>
#include <string>

#include "common/ipc.h"
#include "common/message.h"



namespace casual
{
	namespace statistics
	{
		class Monitor
		{
		public:

			Monitor( const std::vector< std::string>& arguments);
			~Monitor();

			void start();

		private:
			common::ipc::receive::Queue& m_receiveQueue;
		};

		namespace handle
		{
			struct NotifyStats
			{
				typedef common::message::monitor::Notify message_type;

	            void dispatch( message_type& message);

			};
		}
	}
}



#endif /* CASUAL_MONITOR_H_ */
