/*
 * monitordb.h
 *
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */

#ifndef MONITORDB_H_
#define MONITORDB_H_

#include "utility/database.hpp"
#include "common/message.h"

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

				void begin() const;
				void commit() const;

				void insert( const common::message::monitor::Notify& message);
			private:
				//
				// Creates table if necessary
				//
				void createTable();

				database m_database;
			};
		}

	}
}

void operator>>( const casual::common::message::monitor::Notify& message, casual::statistics::monitor::MonitorDB& db);


#endif /* MONITORDB_H_ */
