//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/chronology.h"
#include "common/argument.h"
#include "common/process.h"
#include "common/event/listen.h"
#include "common/exception/handle.h"
#include "common/communication/instance.h"

#include "sql/database.h"


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
                  if ( ! std::uncaught_exception())
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

               void log( common::message::event::service::Call& event)
               {
                  m_connection.execute( "INSERT INTO call VALUES (?,?,?,?,?,?);",
                     event.service,
                     event.parent,
                     event.execution.get(),
                     common::transaction::global( event.trid),
                     event.start,
                     event.end);
               }

            private:
               sql::database::Connection m_connection;
            };


            void main(int argc, char **argv)
            {
               //
               // get database from arguments
               //
               std::string database{"monitor.db"};
               {
                  casual::common::argument::Parse parse{ "service monitor",
                     casual::common::argument::Option( std::tie( database), { "-db", "--database"}, "path to monitor database log")
                  };

                  parse( argc, argv);
               }

               //
               // connect to domain
               //
               common::communication::instance::connect( common::Uuid{ "8130b1cd7e8842a49e3da91f8913aff7"});

               {
                  Handler handler{ database};

                  common::event::idle::listen(
                        [&](){
                        //
                        // the queue is empty
                        //
                        handler.idle();
                     },
                     [&]( common::message::event::service::Call& event){
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
   try
   {
      casual::event::service::main( argc, argv);
      return 0;
   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }
}


