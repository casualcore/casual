/*
 * casual_monitor.h
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#ifndef CASUAL_MONITOR_H_
#define CASUAL_MONITOR_H_

#include <vector>
#include <string>

#include "common/ipc.h"
#include "common/message/monitor.h"

#include "monitor/monitordb.h"



namespace casual
{
	namespace statistics
	{
		namespace monitor
		{
			class Monitor
			{
			public:

				Monitor( const std::vector< std::string>& arguments);
				~Monitor();


				//!
				//! Start the server
				//!
				void start();

			private:
				common::ipc::receive::Queue& m_receiveQueue;
				MonitorDB& m_monitordb;
			};

			namespace handle
			{
				//!
				//! Used with the dispatch handler to handle relevant
				//! messages from queue
				//!
				struct Notify
				{
				public:
					typedef common::message::monitor::Notify message_type;
					Notify( MonitorDB& db_ ) : monitorDB( db_)
					{
					};

					void operator () ( const message_type& message);

				private:
					MonitorDB& monitorDB;

				};
			}
		}
	}
}



#endif /* CASUAL_MONITOR_H_ */
