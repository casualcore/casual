//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

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

               result.groups.push_back( std::move( group));

            }

            {
               broker::State::Group group;
               group.process.pid = 2;
               group.process.queue = 2;
               group.name = "group2";

               result.groups.push_back( std::move( group));
            }

            return result;
         }
      } // local


      TEST( casual_queue_transform, groups_expect_2_groups)
      {
         common::unittest::Trace trace;

         auto groups = transform::groups( local::state());

         ASSERT_TRUE( groups.size() == 2) << CASUAL_MAKE_NVP( groups);

      }



   } // queue
} // casual
