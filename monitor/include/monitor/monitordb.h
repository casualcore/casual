/*
 * monitordb.h
 *
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */

#ifndef MONITORDB_H_
#define MONITORDB_H_

#include "database/database.hpp"
#include "common/message.h"
#include "monitor/monitor_vo.h"

namespace casual
{
	namespace statistics
	{
		namespace monitor
		{
			class MonitorDB
			{
			public:

				MonitorDB();

				MonitorDB( const std::string& database);

				~MonitorDB();

				void insert( const common::message::monitor::Notify& message);
				std::vector< vo::MonitorVO> select( );

				database& getDatabase();
			private:
				//
				// Creates table if necessary
				//
				void createTable();

				database m_database;
			};

			struct Transaction
			{
				Transaction( MonitorDB& db);
				~Transaction();

				MonitorDB& m_monitordb;
			};

		}

	}
}

void operator>>( const casual::common::message::monitor::Notify& message, casual::statistics::monitor::MonitorDB& db);


#endif /* MONITORDB_H_ */
