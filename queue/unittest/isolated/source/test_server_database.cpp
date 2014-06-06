//!
//! test_server_database.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "queue/server/database.h"


#include "common/file.h"


namespace casual
{
   namespace queue
   {

      namespace local
      {
         namespace
         {
            common::file::ScopedPath file()
            {
               return common::file::ScopedPath{ "unittest_queue_server_database.db"};
            }
         } // <unnamed>
      } // local

      TEST( casual_queue_server_database, create_queue)
      {
         auto path = local::file();
         server::Database database( path);

         database.create( "unittest_queue");

         auto queues = database.queues();

         ASSERT_TRUE( queues.size() == 1);
         EXPECT_TRUE( queues.at( 0) == "unittest_queue");
      }


      TEST( casual_queue_server_database, create_100_queues)
      {
         auto path = local::file();
         server::Database database( path);

         auto count = 0;

         while( count++ < 100)
         {
            database.create( "unittest_queue" + std::to_string( count));
         }

         auto queues = database.queues();

         ASSERT_TRUE( queues.size() == 100) << "size: " << queues.size();
         EXPECT_TRUE( queues.at( 99) == "unittest_queue99");
      }


   } // queue
} // casual
