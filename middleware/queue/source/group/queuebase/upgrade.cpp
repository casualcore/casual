//!
//! Copyright (c) 2026, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/group/queuebase/upgrade.h"
#include "queue/group/queuebase/schema.h"

#include "common/execute.h"
#include "common/algorithm/sorted.h"

#include "sql/database.h"

namespace casual
{
   namespace queue::group::upgrade
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

            auto tasks()
            {
               return std::vector< Task>{
                  
                  // from 1.x to 2.0
                  {
                     sql::database::Version{ 2, 0},
                     []( sql::database::Connection& connection)
                     {
                        common::Trace trace{ "queue::upgrade::local::global::tasks to version 2.0" };

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

                        // everything went ok, we commit.
                        rollback.release();
                        connection.commit();
                     }
                  },
                  // from 2.0 to 3.0
                  {
                     sql::database::Version{ 3, 0},
                     []( sql::database::Connection& connection)
                     {
                        common::Trace trace{ "queue::upgrade::local::global::tasks to version 3.0" };
                        
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

                        // everything went ok, we commit.
                        rollback.release();
                        connection.commit();
                     }
                  },
                  // from 3.0 to 4.0
                  {
                     sql::database::Version{ 4, 0},
                     []( sql::database::Connection& connection)
                     {
                        common::Trace trace{ "queue::upgrade::local::global::tasks to version 4.0" };

                        connection.exclusive_begin();

                        auto rollback = common::execute::scope( [ &connection](){ connection.rollback();});

                        connection.statement( R"( ALTER TABLE message ADD COLUMN header TEXT NULL; )");

                        sql::database::version::set( connection, { 4, 0});

                        rollback.release();
                        connection.commit();
                     }

                  }

               };
            } 
         } // unnamed
      } // local

      void queuebase( const std::filesystem::path& queuebase)
      {
         common::log::debug( "queuebase: ", queuebase);

         sql::database::Connection connection{ queuebase};
         
         auto version = sql::database::version::get( connection);

         auto run_task = [ &connection, &queuebase]( auto& task)
         {
            common::log::information( "upgrade '", queuebase, "' to version: ", task.to);
            task.action( connection);
         };

         auto tasks = local::tasks();

         common::algorithm::for_each( 
            // get the 'right' range of upper_bound and upgrade with all steps that's necessary
            std::get< 1>( common::algorithm::sorted::upper_bound( tasks, version)), 
            run_task);
      }
      
   } // queue::group::upgrade
   
} // casual
