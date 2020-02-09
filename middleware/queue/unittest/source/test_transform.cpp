//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "queue/common/transform.h"
#include "queue/manager/manager.h"

#include "serviceframework/log.h"


namespace casual
{
   namespace queue
   {
      namespace local
      {
         manager::State state()
         {
            manager::State result;

            {
               manager::State::Group group;
               group.process.pid = common::strong::process::id{ 1};
               group.process.ipc = common::strong::ipc::id{ common::uuid::make()};
               group.name = "group1";

               result.groups.push_back( std::move( group));

            }

            {
               manager::State::Group group;
               group.process.pid = common::strong::process::id{ 2};
               group.process.ipc = common::strong::ipc::id{ common::uuid::make()};
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

         ASSERT_TRUE( groups.size() == 2) << CASUAL_NAMED_VALUE( groups);

      }



   } // queue
} // casual
