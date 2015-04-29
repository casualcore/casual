//!
//! test_queue.cpp
//!
//! Created on: Jul 1, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "config/queue.h"

#include "common/exception.h"


#include "sf/log.h"


namespace casual
{
   namespace config
   {

      TEST( casual_configuration_queue, expect_two_groups)
      {
         auto queues = config::queue::get( "queue.yaml");

         EXPECT_TRUE( queues.groups.size() == 2) << CASUAL_MAKE_NVP( queues);

      }


      TEST( casual_configuration_queue, expect_4_queues_in_group_one)
      {
         auto queues = config::queue::get( "queue.yaml");

         EXPECT_TRUE( queues.groups.at( 0).name == "someGroup");
         EXPECT_TRUE( queues.groups.at( 0).queuebase == "some-group.qb") <<  queues.groups.at( 0).queuebase;
         EXPECT_TRUE( queues.groups.at( 0).queues.size() == 4) << CASUAL_MAKE_NVP( queues);

      }


      TEST( casual_configuration_queue, validate__group_has_to_have_a_name)
      {
         queue::Domain domain;
         domain.groups.resize( 1);

         EXPECT_THROW( { queue::unittest::validate( domain);}, common::exception::invalid::Configuration);
      }


      TEST( casual_configuration_queue, validate__group_has_to_have_unique_name)
      {
         queue::Domain domain;
         domain.groups.resize( 2);
         domain.groups.at( 0).name = "A";
         domain.groups.at( 0).queuebase = "X";
         domain.groups.at( 1).name = "A";
         domain.groups.at( 1).queuebase = "Z";


         EXPECT_THROW( { queue::unittest::validate( domain);}, common::exception::invalid::Configuration);
      }

      TEST( casual_configuration_queue, validate__group_has_to_have_unique_queuebase)
      {
         queue::Domain domain;
         domain.groups.resize( 2);
         domain.groups.at( 0).name = "A";
         domain.groups.at( 0).queuebase = "X";
         domain.groups.at( 1).name = "B";
         domain.groups.at( 1).queuebase = "X";


         EXPECT_THROW( { queue::unittest::validate( domain);}, common::exception::invalid::Configuration);
      }

      TEST( casual_configuration_queue, validate__queue_has_to_have_unique_name)
      {
         queue::Domain domain;
         domain.groups.resize( 1);
         domain.groups.at( 0).name = "A";
         domain.groups.at( 0).queuebase = "X";
         domain.groups.at( 0).queues.resize( 2);
         domain.groups.at( 0).queues.at( 0).name = "a";
         domain.groups.at( 0).queues.at( 1).name = "a";


         EXPECT_THROW( { queue::unittest::validate( domain);}, common::exception::invalid::Configuration);
      }

      TEST( casual_configuration_queue, validate__queue_has_to_have_unique_name_regardless_of_group)
      {
         queue::Domain domain;
         domain.groups.resize( 2);
         domain.groups.at( 0).name = "A";
         domain.groups.at( 0).queuebase = "X";
         domain.groups.at( 0).queues.resize( 1);
         domain.groups.at( 0).queues.at( 0).name = "a";

         domain.groups.at( 1).name = "B";
         domain.groups.at( 1).queuebase = "Y";
         domain.groups.at( 1).queues.resize( 1);
         domain.groups.at( 1).queues.at( 0).name = "a";

         EXPECT_THROW( { queue::unittest::validate( domain);}, common::exception::invalid::Configuration);
      }

      TEST( casual_configuration_queue, default_values__retries)
      {
         queue::Domain domain;
         domain.casual_default.queue.retries = "42";
         domain.groups.resize( 1);
         domain.groups.at( 0).name = "A";
         domain.groups.at( 0).queuebase = "X";
         domain.groups.at( 0).queues.resize( 1);
         domain.groups.at( 0).queues.at( 0).name = "a";

         queue::unittest::default_values( domain);
         EXPECT_TRUE( domain.groups.at( 0).queues.at( 0).retries == "42") << domain.groups.at( 0).queues.at( 0).retries;
      }


   } // config

} // casual
