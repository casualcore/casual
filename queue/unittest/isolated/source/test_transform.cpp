//!
//! test_transform.cpp
//!
//! Created on: Oct 4, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "queue/transform.h"
#include "queue/broker/broker.h"

#include "sf/log.h"


namespace casual
{
   namespace queue
   {
      namespace local
      {
         broker::State state()
         {
            broker::State result;

            {
               broker::State::Group group;
               group.id.pid = 1;
               group.id.queue_id = 1;
               group.name = "group1";


               common::message::queue::lookup::Reply reply{ group.id, 3};

               result.queues.emplace( "q11", reply);
               result.queues.emplace( "q12", reply);
               result.queues.emplace( "q13", reply);

               result.groups.push_back( std::move( group));

            }

            {
               broker::State::Group group;
               group.id.pid = 2;
               group.id.queue_id = 2;
               group.name = "group2";

               common::message::queue::lookup::Reply reply{ group.id, 2};

               result.queues.emplace( "q21", reply);
               result.queues.emplace( "q22", reply);
               result.queues.emplace( "q23", reply);

               result.groups.push_back( std::move( group));
            }

            return result;
         }
      } // local


      TEST( casual_queue_transform, groups_expect_2_groups)
      {
         auto groups = transform::groups( local::state());

         ASSERT_TRUE( groups.size() == 2) << CASUAL_MAKE_NVP( groups);

      }


      TEST( casual_queue_transform, groups_expect_2_groups_with_3_queues)
      {
         auto groups = transform::groups( local::state());

         ASSERT_TRUE( groups.size() == 2) << CASUAL_MAKE_NVP( groups);
         EXPECT_TRUE( groups.at( 0).queues.size() == 3) << CASUAL_MAKE_NVP( groups);
         EXPECT_TRUE( groups.at( 1).queues.size() == 3) << CASUAL_MAKE_NVP( groups);

      }

   } // queue
} // casual
