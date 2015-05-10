/*
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */

#include "common/trace.h"
#include "common/environment.h"
#include "common/chronology.h"

#include "sql/database.h"

#include "receiver.h"


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


         struct Handler : handler::Base
         {
            Handler( const std::string& value) : m_connection( sql::database::Connection( value))
            {
               std::ostringstream stream;
               stream   << "CREATE TABLE IF NOT EXISTS calls ( "
                        << "service       TEXT, "
                        << "parentservice TEXT, "
                        << "callid        TEXT, " // should not be string
                        << "transactionid BLOB, "
                        << "start         NUMBER, "
                        << "end           NUMBER);";

               m_connection.execute( stream.str());
            }

            void persist_begin( ) override
            {
               m_connection.begin();
            }


            void log( const message_type& message) override
            {
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

            void persist_commit() override
            {
               if ( ! std::uncaught_exception())
               {
                  m_connection.commit();
               }
               else
               {
                  m_connection.rollback();
               }
            }

         private:
            sql::database::Connection m_connection;
         };

      } // monitor
   } // traffic
} // casual



int main( int argc, char **argv)
{


   // get log-file from arguments
   std::string database = casual::traffic::monitor::local::getDatabase();

   // set process name from arguments



   casual::traffic::monitor::Handler handler{ database};

   casual::traffic::Receiver receive;
   return receive.start( handler);
}


