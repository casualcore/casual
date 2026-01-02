//!
//! Copyright (c) 2026, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "queue/group/queuebase/schema.h"
#include "queue/group/queuebase/upgrade.h"
#include "queue/group/queuebase.h"

#include "common/uuid.h"

#include "sql/database.h"

namespace casual
{
   namespace queue::group
   {
      TEST( queue_group_queuebase_upgrade, v3_0__to__v4_0)
      {
         common::unittest::Trace trace;

         auto qb = common::file::scoped::Path{ common::unittest::file::temporary::name( ".qb")};

         // create and prepare a v3.0 database
         {
            
            sql::database::Connection connection{ qb};

            connection.statement( queuebase::schema::table::v3_0::queue);
            connection.statement( queuebase::schema::table::v3_0::message);
            sql::database::version::set( connection, sql::database::Version{ 3, 0}); 

            connection.statement( R"(
               INSERT INTO queue ( 
                  id,
                  name, 
                  retry_count, 
                  retry_delay, 
                  error, 
                  count, 
                  size, 
                  uncommitted_count, 
                  metric_dequeued, 
                  metric_enqueued, 
                  last, 
                  created) 
               VALUES ( 
                  1, 'a', 5, 1000, 1, 0, 0, 0, 0, 0, 0, 0
               );
               )");

            connection.statement( R"(
               INSERT INTO message ( 
                  id,
                  queue,
                  origin,
                  gtrid,
                  properties,
                  state,
                  reply,
                  redelivered,
                  type,
                  available,
                  timestamp,
                  payload
                 ) 
               VALUES (
                  X'FFFFFFFF', 1, 1, NULL, '', 2, 'reply-queue', 0, 'json/', 0, 0, X'44556677'
               );
            )");
         }

         // upgrade
         {
            sql::database::Connection connection{ qb};
            EXPECT_TRUE(( sql::database::version::get( connection) == sql::database::Version{ 3, 0}));

            // upgrade to v4.0
            queue::group::upgrade::queuebase( qb);
            EXPECT_TRUE(( sql::database::version::get( connection) == sql::database::Version{ 4, 0}));

            // make sure we've got the added header column
            std::ostringstream out;
            connection.statement( R"(
               SELECT name FROM PRAGMA_TABLE_INFO( 'message') WHERE name='header';
            )", out);

            EXPECT_TRUE( out.str() == "header|\n") << out.str();
         }

         // some sanity checks
         {
            queue::group::Queuebase queuebase{ qb};

            auto request = queue::ipc::message::group::dequeue::Request{};
            {
               request.queue = common::strong::queue::id{ 1};
            }

            auto result = queuebase.dequeue( 
               request,
               common::chronology::time_point::clock::now());

            ASSERT_TRUE( result.message);
            EXPECT_TRUE( result.message->payload.type == "json/") << CASUAL_NAMED_VALUE( result.message->payload.type);
         }


      }

      
   } // queue::group
   
} // casual
