//!
//! test_queue.cpp
//!
//! Created on: Jul 1, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "config/queue.h"


namespace casual
{
   namespace config
   {

      TEST( casual_configuration_queue, expect_two_groups)
      {
         auto queues = config::queue::get( "queue.yaml");

         EXPECT_TRUE( queues.groups.size() == 2);

      }


      TEST( casual_configuration_queue, expect_4_queues_in_group_one)
      {
         auto queues = config::queue::get( "queue.yaml");

         EXPECT_TRUE( queues.groups.at( 0).name == "someGroup");
         EXPECT_TRUE( queues.groups.at( 0).queuebase == "some-group.qb");
         EXPECT_TRUE( queues.groups.at( 0).queues.size() == 4);

      }





   } // config



} // casual
