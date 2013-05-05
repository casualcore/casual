/*
 * monitordb.cpp
 *
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */
#include "monitor/monitordb.h"
#include "common/trace.h"
#include <vector>
#include <sstream>
#include <iostream>
#include <string>
#include "common/environment.h"

//
// TODO: Use casual exception
//
#include <stdexcept>

namespace casual
{
namespace statistics
{
namespace monitor
{

	namespace local
	{
		namespace
		{
			std::string getDatabase()
			{
				return common::environment::variable::get("CASUAL_ROOT") + "/monitor.db";
			}
		}
	}

	MonitorDB::MonitorDB() : MonitorDB( local::getDatabase())
	{
		static const std::string cMethodname("MonitorDB::MonitorDB()");
		common::Trace trace(cMethodname);
	}

	MonitorDB::MonitorDB( const std::string& database) : m_database( database)
	{
		static const std::string cMethodname("MonitorDB::MonitorDB(...)");
		common::Trace trace(cMethodname);

		createTable();
	}

	MonitorDB::~MonitorDB( )
	{
		static const std::string cMethodname("MonitorDB::~MonitorDB");
		common::Trace trace(cMethodname);
	}

	void MonitorDB::begin() const
	{
		static const std::string cMethodname("MonitorDB::begin()");
		common::Trace trace(cMethodname);
		m_database.begin();
	}

	void MonitorDB::commit() const
	{
		static const std::string cMethodname("MonitorDB::commit()");
		common::Trace trace(cMethodname);
		m_database.commit();
	}


	void MonitorDB::createTable()
	{
		static const std::string cMethodname("MonitorDB::createTable");
		common::Trace trace(cMethodname);

		std::ostringstream stream;
		stream << "CREATE TABLE IF NOT EXISTS calls ( "
			   << "service			TEXT, "
               << "parentservice	TEXT, "
               << "callid			TEXT, "
               << "transactionid	BLOB, "
               << "start			NUMBER, "
               << "end				NUMBER, "
               << "PRIMARY KEY (start, callid));";

		if ( !m_database.sql( stream.str()))
		{
			throw std::runtime_error( m_database.error());
		}

	}

	void MonitorDB::insert( const common::message::monitor::Notify& message)
	{
		static const std::string cMethodname("MonitorDB::insert");
		common::Trace trace(cMethodname);

		std::ostringstream stream;
		stream << "INSERT INTO calls VALUES (?,?,?,?,?,?);";
		if ( !m_database.sql( stream.str(),
				message.service,
				message.parentService,
				message.callId.string(),
				message.transactionId,
				std::chrono::time_point_cast<std::chrono::microseconds>(message.start).time_since_epoch().count(),
				std::chrono::time_point_cast<std::chrono::microseconds>(message.end).time_since_epoch().count()))
		{
			throw std::runtime_error( m_database.error());
		}

	}
}
}
}

void operator>>( const casual::common::message::monitor::Notify& message, casual::statistics::monitor::MonitorDB& db)
{
	db.insert( message);
}


