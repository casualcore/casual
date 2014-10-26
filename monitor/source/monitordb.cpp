/*
 * monitordb.cpp
 *
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */
#include "monitor/monitordb.h"
#include "common/trace.h"
#include "common/environment.h"
#include "common/chronology.h"


#include <vector>
#include <sstream>
#include <iostream>
#include <string>
#include <cstdlib>


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
				return common::environment::directory::domain() + "/monitor.db";
			}

			std::string getValue( database::Row& row, const std::string& attribute)
			{
				std::string value;
				if ( !row[attribute].empty())
				{
					value = row[attribute].front();
				}
				common::log::debug << "getValue(" << attribute << "): " << value << std::endl;
				return value;
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

	Transaction::Transaction( MonitorDB& monitordb) : m_monitordb( monitordb)
	{
		static const std::string cMethodname("MonitorDB::begin()");
		common::Trace trace(cMethodname);
		m_monitordb.getDatabase().begin();
	}

	Transaction::~Transaction()
	{
		static const std::string cMethodname("Transaction::~Transaction()");
		common::Trace trace(cMethodname);
		if ( ! std::uncaught_exception())
		{
			m_monitordb.getDatabase().commit();
		}
		else
		{
			m_monitordb.getDatabase().rollback();
		}
	}


	void MonitorDB::createTable()
	{
		static const std::string cMethodname("MonitorDB::createTable");
		common::Trace trace(cMethodname);

		std::ostringstream stream;
		stream << "CREATE TABLE IF NOT EXISTS calls ( "
			   << "service			TEXT, "
               << "parentservice	TEXT, "
               << "callid			TEXT, " // should not be string
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
				common::uuid::string( message.callId), // should not be string
				message.transactionId,
				std::chrono::time_point_cast<std::chrono::microseconds>(message.start).time_since_epoch().count(),
				std::chrono::time_point_cast<std::chrono::microseconds>(message.end).time_since_epoch().count()))
		{
			throw std::runtime_error( m_database.error());
		}

	}

	std::vector< vo::MonitorVO> MonitorDB::select( )
	{
		static const std::string cMethodname("MonitorDB::select");
		common::Trace trace(cMethodname);

		std::ostringstream stream;
		stream << "SELECT service, parentservice, callid, transactionid, start, end FROM calls;";
		std::vector< database::Row> rows;
		if ( !m_database.sql( stream.str(), rows))
		{
			throw std::runtime_error( m_database.error());
		}

 		std::vector< vo::MonitorVO> result;
     	for(auto row = rows.begin(); row != rows.end(); row++)
     	{
     		vo::MonitorVO vo;
     		vo.setSrv( local::getValue( *row, "service"));
     		vo.setParentService( local::getValue( *row, "parentservice"));
     		sf::platform::Uuid callId( local::getValue( *row, "callid"));
     		vo.setCallId( callId);
     		//vo.setTransactionId( local::getValue( *row, "transactionid"));

     		std::chrono::microseconds start{ strtoll(local::getValue( *row,"start").c_str(), 0, 10)};
     		vo.setStart( common::platform::time_point{ start});
     		std::chrono::microseconds end{ strtoll(local::getValue( *row,"end").c_str(), 0, 10)};
     		vo.setEnd( common::platform::time_point{ end});
			result.push_back( vo);
		}

		return result;
	}

	database& MonitorDB::getDatabase()
	{
		return m_database;
	}
}
}
}

void operator>>( const casual::common::message::monitor::Notify& message, casual::statistics::monitor::MonitorDB& db)
{
	db.insert( message);
}


