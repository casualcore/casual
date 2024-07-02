//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/chronology.h"
#include "common/argument.h"
#include "common/process.h"
#include "common/algorithm.h"
#include "common/event/listen.h"
#include "common/exception/guard.h"
#include "common/communication/instance.h"
#include "common/uuid.h"

#include "sql/database.h"


#include <vector>
#include <sstream>
#include <iostream>
#include <string>
#include <cstdlib>

// TODO: Use casual exception
#include <stdexcept>

namespace casual
{
   namespace event
   {
      namespace service
      {
         namespace
         {
            struct Handler
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

                  m_connection.begin();
               }

               ~Handler()
               {
                  if ( ! std::uncaught_exceptions())
                  {
                     m_connection.commit();
                  }
                  else
                  {
                     m_connection.rollback();
                  }
               }

               void idle( )
               {
                  m_connection.commit();
                  m_connection.begin();
               }

               void log( common::message::event::service::Metric& metric)
               {
                  m_connection.execute( "INSERT INTO call VALUES (?,?,?,?,?,?);",
                     metric.service,
                     metric.parent.service,
                     metric.execution.underlying().range(),
                     common::transaction::id::range::global( metric.trid),
                     metric.start,
                     metric.end);
               }


               void log( common::message::event::service::Calls& event)
               {
                  common::algorithm::for_each( event.metrics, [&]( auto& metric){ log( metric);});
               }

            private:
               sql::database::Connection m_connection;
            };


            void main(int argc, char **argv)
            {
               // get database from arguments
               std::string database{"monitor.db"};
               {
                  casual::common::argument::Parse parse{ "service monitor",
                     casual::common::argument::Option( std::tie( database), { "-db", "--database"}, "path to monitor database log")
                  };

                  parse( argc, argv);
               }

               // connect to domain
               common::communication::instance::whitelist::connect( 0x8130b1cd7e8842a49e3da91f8913aff7_uuid);

               {
                  Handler handler{ database};

                  common::event::listen(
                     common::event::condition::compose( common::event::condition::idle( [&handler]()
                     {
                        // inbound is idle, 
                        handler.idle();
                     })),
                     [&]( common::message::event::service::Calls& event)
                     {
                        handler.log( event);
                     });
               }
            }
         }

      } // service
   } // event

} // casual



int main( int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::event::service::main( argc, argv);
   });
}


