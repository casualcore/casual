//! 
//! Copyright (c) 2010, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/common/log.h"
#include "queue/group/queuebase/schema.h"

#include "common/exception/capture.h"
#include "casual/argument.h"
#include "common/algorithm.h"
#include "common/algorithm/sorted.h"
#include "common/execute.h"

#include "sql/database.h"

#include <iostream>

namespace casual
{
   using namespace common;
   namespace queue::upgrade
   {
      namespace local
      {
         namespace
         {
            struct Task
            {
               sql::database::Version to;
               std::function< void( sql::database::Connection&)> action;

               //inline friend bool operator < ( const Task& lhs, const sql::database::Version& rhs) { return lhs.to < rhs;}
               inline friend bool operator < ( const sql::database::Version& lhs, const Task& rhs) { return lhs < rhs.to;}
            };

            namespace global
            {
               auto const tasks = std::vector< Task>{
                  
                  // from 1.x to 2.0
                  {
                     sql::database::Version{ 2, 0},
                     []( sql::database::Connection& connection)
                     {
                        Trace trace{ "queue::upgrade::local::global::tasks to version 2.0" };

                        // disable FK
                        connection.statement( "PRAGMA foreign_keys = OFF;");

                        connection.exclusive_begin();

                        auto rollback = common::execute::scope( [&connection](){ connection.rollback();});

                        // we need to upgrade queue
                        {
                           // first we rename the current to queue_v1
                           connection.statement( "ALTER TABLE queue RENAME TO queue_v1;");

                           // create the new table
                           connection.statement( group::queuebase::schema::table::queue);
                           
                           // migrate data
                           connection.statement( R"(
INSERT INTO queue 
   SELECT
      id,  
      name,  
      retries,  
      0,  -- retry.delay
      CASE WHEN type = 3 THEN error ELSE 0 END,
      count,  
      size,  
      uncommitted_count,
      timestamp
   FROM
      queue_v1
;
)");
                              
                           // drop the old table
                           connection.statement( "DROP TABLE queue_v1;");
                        }

                        // alter message.avalible to message.available
                        {
                           // not brand new versions of sqlite does not support renaming columns so 
                           // we ned to do the v1 dance for this also.
                           
                           // first we rename the current to queue_v1
                           connection.statement( "ALTER TABLE message RENAME TO message_v1;");

                           // create the new table
                           connection.statement( group::queuebase::schema::table::message);

                           // migrate data
                           connection.statement( R"(
INSERT INTO message 
   SELECT
      id          ,
      queue       ,
      origin      , 
      gtrid       ,
      properties  ,
      state       , 
      reply       ,
      redelivered ,
      type        ,
      avalible    ,  -- is renamed to availiable
      timestamp   ,
      payload 
   FROM
      message_v1
;
)");
                           // drop the old table
                           connection.statement( "DROP TABLE message_v1;");
                        }

                        sql::database::version::set( connection, sql::database::Version{ 2, 0});

                        // everythin went ok, we commit.
                        rollback.release();
                        connection.commit();
                     }
                  },
                  // from 2.0 to 3.0
                  {
                     sql::database::Version{ 3, 0},
                     []( sql::database::Connection& connection)
                     {
                        Trace trace{ "queue::upgrade::local::global::tasks to version 3.0" };
                        // disable FK
                        connection.statement( "PRAGMA foreign_keys = OFF;");

                        connection.exclusive_begin();

                        auto rollback = common::execute::scope( [&connection](){ connection.rollback();});

                        // drop all triggers (will be created on startup)
                        connection.statement( group::queuebase::schema::drop::triggers);

                        // we need to upgrade queue
                        {
                           // first we rename the current to queue_v2
                           connection.statement( "ALTER TABLE queue RENAME TO queue_v2;");

                           // create the new table
                           connection.statement( group::queuebase::schema::table::queue);

                           // migrate data
                           // julianday('now') - 2440587.5) *86400.0 <- some magic that sqlite recommend for fraction of seconds
                           connection.statement( R"(
INSERT INTO queue 
   SELECT
      id,  
      name,  
      retry_count,  
      retry_delay,
      error,
      count,  
      size,  
      uncommitted_count,
      0, -- metric_dequeued    
      0, -- metric_enqueued
      timestamp, -- last
      ( julianday('now') - 2440587.5) *86400 * 1000 * 1000   -- created
   FROM
      queue_v2
;
)");

                              // drop the old table
                           connection.statement( "DROP TABLE queue_v2;");
                        }
                        sql::database::version::set( connection, sql::database::Version{ 3, 0});

                        // everythin went ok, we commit.
                        rollback.release();
                        connection.commit();
                     }
                  }

               };
            } // global

            void file( const std::string& file)
            {
               common::log::line( queue::log, "file: ", file);

               sql::database::Connection connection{ file};
               
               auto version = sql::database::version::get( connection);

               auto run_task = [&connection, &file]( auto& task)
               {
                  std::cout << "upgrade '" << file << "' to version: " << task.to << '\n';
                  task.action( connection);
               };

               algorithm::for_each( 
                  // get the 'right' range of upper_bound and upgrade with all steps that's necessary
                  std::get< 1>( algorithm::sorted::upper_bound( global::tasks, version)), 
                  run_task);
            }

            void main( int argc, const char** argv)
            {
               std::vector< std::string> files;

               argument::parse( "upgrades queue-base files to latest version", {
                  argument::Option{ argument::option::one::many( files), { "-f", "--files"}, "queue-base files to upgrade" }( argument::cardinality::one())
               }, argc, argv);
               
               algorithm::for_each( files, &local::file);
            }
         } // <unnamed>
      } // local

      
   } // queue::update
} // casual


int main( int argc, const char** argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::queue::upgrade::local::main( argc, argv);
   });
}
