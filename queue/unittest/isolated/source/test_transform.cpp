//!
//! test_transform.cpp
//!
//! Created on: Oct 4, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "queue/common/transform.h"
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
               group.process.pid = 1;
               group.process.queue = 1;
               group.name = "group1";


               common::message::queue::lookup::Reply reply{ group.process, 3};

               result.queues.emplace( "q11", reply);
               result.queues.emplace( "q12", reply);
               result.queues.emplace( "q13", reply);

               result.groups.push_back( std::move( group));

            }

            {
               broker::State::Group group;
               group.process.pid = 2;
               group.process.queue = 2;
               group.name = "group2";

               common::message::queue::lookup::Reply reply{ group.process, 2};

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



   } // queue
} // casual
