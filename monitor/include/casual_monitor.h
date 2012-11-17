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

#include "casual_ipc.h"



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
			ipc::send::Queue& m_brokerQueue;
			ipc::receive::Queue& m_receiveQueue;
		};

		class Context
		{
		public:
			static Context& instance();
			ipc::send::Queue& brokerQueue();
			ipc::receive::Queue& receiveQueue();
		private:
			Context();

			ipc::send::Queue m_brokerQueue;
			ipc::receive::Queue m_receiveQueue;
		};

	}
}



#endif /* CASUAL_MONITOR_H_ */
