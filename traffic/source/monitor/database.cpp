/*
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */
#include "traffic/monitor/database.h"

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
namespace traffic
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
		}
	}

	Database::Database() : Database( local::getDatabase())
	{
		static const std::string cMethodname("Database::Database()");
		common::Trace trace(cMethodname);
	}

	Database::Database( const std::string& database) : m_connection( sql::database::Connection( database))
	{
		static const std::string cMethodname("Database::Database(...)");
		common::Trace trace(cMethodname);

		createTable();
	}

	Database::~Database( )
	{
		static const std::string cMethodname("Database::~Database");
		common::Trace trace(cMethodname);
	}

	Transaction::Transaction(Database& database) : m_database( database)
	{
		static const std::string cMethodname("Database::begin()");
		common::Trace trace(cMethodname);
		m_database.getConnection().begin();
	}

	Transaction::~Transaction()
	{
		static const std::string cMethodname("Transaction::~Transaction()");
		common::Trace trace(cMethodname);
		if ( ! std::uncaught_exception())
		{
		   m_database.getConnection().commit();
		}
		else
		{
		   m_database.getConnection().rollback();
		}
	}


	void Database::createTable()
	{
		static const std::string cMethodname("Database::createTable");
		common::Trace trace(cMethodname);

		std::ostringstream stream;
		stream   << "CREATE TABLE IF NOT EXISTS calls ( "
			      << "service			TEXT, "
               << "parentservice	TEXT, "
               << "callid			TEXT, " // should not be string
               << "transactionid	BLOB, "
               << "start			NUMBER, "
               << "end				NUMBER);";

		m_connection.execute( stream.str());
	}

	void Database::store( const common::message::traffic::monitor::Notify& message)
	{
		static const std::string cMethodname("Database::store");
		common::Trace trace(cMethodname);

		std::ostringstream stream;
		stream << "INSERT INTO calls VALUES (?,?,?,?,?,?);";
		m_connection.execute( stream.str(),
				message.service,
				message.parentService,
				common::uuid::string( message.callId), // should not be string
				message.transactionId,
				std::chrono::time_point_cast<std::chrono::microseconds>(message.start).time_since_epoch().count(),
				std::chrono::time_point_cast<std::chrono::microseconds>(message.end).time_since_epoch().count());
	}

	std::vector< ServiceEntryVO> Database::select( )
	{
		static const std::string cMethodname("Database::select");
		common::Trace trace(cMethodname);

		std::ostringstream stream;
		stream << "SELECT service, parentservice, callid, transactionid, start, end FROM calls;";
		sql::database::Statement::Query query = m_connection.query( stream.str());

		sql::database::Row row;
		std::vector< ServiceEntryVO> result;

      while( query.fetch( row))
      {
     		ServiceEntryVO vo;
     		vo.setService( row.get< std::string>(0));
     		vo.setParentService( row.get< std::string>( 1));
     		sf::platform::Uuid callId( row.get< std::string>( 2));
     		vo.setCallId( callId);
     		//vo.setTransactionId( local::getValue( *row, "transactionid"));

     		std::chrono::microseconds start{ row.get< long long>( 4)};
     		vo.setStart( common::platform::time_point{ start});
     		std::chrono::microseconds end{ row.get< long long>( 5)};
     		vo.setEnd( common::platform::time_point{ end});
			result.push_back( vo);
		}

		return result;
	}

	sql::database::Connection& Database::getConnection()
	{
		return m_connection;
	}
}
}
}
void operator>>( const casual::common::message::traffic::monitor::Notify& message, casual::traffic::monitor::Database& db)
{
	db.store( message);
}


