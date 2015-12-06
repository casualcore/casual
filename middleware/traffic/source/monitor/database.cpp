/*
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */

#include "common/trace.h"
#include "common/chronology.h"
#include "common/arguments.h"
#include "common/process.h"


#include "sql/database.h"

#include "traffic/receiver.h"


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
   struct Handler : handler::Base
   {
      Handler( const std::string& value) : m_connection( sql::database::Connection( value))
      {
         m_connection.execute(
             R"( CREATE TABLE IF NOT EXISTS call
             (
               service       TEXT NOT NULL,
               parent        TEXT,
               execution     BLOB  NOT NULL,
               xid           BLOB,
               start         NUMBER  NOT NULL,
               end           NUMBER NOT NULL
               ); 
             )");

         m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_service ON call ( parent, service);" );

         m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_execution ON call ( execution);" );

         m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_xid ON call ( xid);" );
      }

      void persist_begin( ) override
      {
         m_connection.begin();
      }

      void log( const event_type& event) override
      {
         m_connection.execute( "INSERT INTO call VALUES (?,?,?,?,?,?);",
            event.service(),
            event.parent(),
            event.execution().get(),
            common::transaction::global( event.transaction()),
            event.start(),
            event.end());
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

   try
   {
      // get database from arguments
      std::string database{"monitor.db"};
      {
         casual::common::Arguments parser{
            { casual::common::argument::directive( { "-db", "--database"}, "path to monitor database log", database)}};

         parser.parse( argc, argv);
      }

      casual::traffic::monitor::Handler handler{ database};

      casual::traffic::Receiver receive{ casual::common::Uuid{ "8130b1cd7e8842a49e3da91f8913aff7"}};
      return receive.start( handler);
   }
   catch( ...)
   {
      return casual::common::error::handler();
   }
}


