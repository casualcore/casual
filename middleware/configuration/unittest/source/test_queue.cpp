//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "configuration/domain.h"
#include "configuration/example/domain.h"

#include "common/file.h"

#include "common/code/casual.h"


#include "serviceframework/log.h"


namespace casual
{
   namespace configuration
   {
      class configuration_queue : public ::testing::TestWithParam< const char*>
      {
      };


      INSTANTIATE_TEST_CASE_P( protocol,
            configuration_queue,
         ::testing::Values(".yaml", ".json", ".xml", ".ini"));


      // Look at configuration/example/domain.yaml for what to expect.

      TEST_P( configuration_queue, expect_3_groups)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto queue = domain::get( { example::temporary( example::domain(), GetParam())}).queue;

         EXPECT_TRUE( queue.groups.size() == 3) << CASUAL_NAMED_VALUE( queue);
      }


      TEST_P( configuration_queue, expect_4_queues_in_group_1)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto manager = domain::get( { example::temporary( example::domain(), GetParam())}).queue;

         auto& group =  manager.groups.at( 0);

         EXPECT_TRUE( group.name == "groupA");
         EXPECT_TRUE( group.queuebase.has_value()) << CASUAL_NAMED_VALUE( group);
         ASSERT_TRUE( group.queues.size() == 4) << CASUAL_NAMED_VALUE( group);
         {
            auto& queue = group.queues.at( 0);
            EXPECT_TRUE( queue.name == "q_A1");
            EXPECT_TRUE( queue.retry.value().count == 3L);
            EXPECT_TRUE( queue.retry.value().delay == std::string{ "20s"});
         }

         {
            auto& queue = group.queues.at( 1);
            EXPECT_TRUE( queue.name == "q_A2");
            EXPECT_TRUE( queue.retry.value().count == 10L);
            EXPECT_TRUE( queue.retry.value().delay == std::string{ "100ms"});
         }
      }


      TEST( casual_configuration_queue, validate__group_has_to_have_a_name)
      {
         common::unittest::Trace trace;

         queue::Manager manager;
         manager.groups.resize( 1);

         EXPECT_CODE(
         { 
            queue::unittest::validate( manager);
         }, common::code::casual::invalid_configuration);
      }


      TEST( casual_configuration_queue, validate__group_has_to_have_unique_name)
      {
         common::unittest::Trace trace;

         queue::Manager manager;
         manager.groups.resize( 2);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase.emplace( "X");
         manager.groups.at( 1).name = "A";
         manager.groups.at( 1).queuebase.emplace( "Z");


         EXPECT_CODE(
         { 
            queue::unittest::validate( manager);
         }, common::code::casual::invalid_configuration);
      }

      TEST( casual_configuration_queue, validate__group_has_to_have_unique_queuebase)
      {
         common::unittest::Trace trace;

         queue::Manager manager;
         manager.groups.resize( 2);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase.emplace( "X");
         manager.groups.at( 1).name = "B";
         manager.groups.at( 1).queuebase.emplace( "X");


         EXPECT_CODE(
         { 
            queue::unittest::validate( manager);
         }, common::code::casual::invalid_configuration);
      }

      TEST( casual_configuration_queue, validate__multiple_groups_can_have_memory_queuebase)
      {
         common::unittest::Trace trace;

         queue::Manager manager;
         manager.groups.resize( 2);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase.emplace( ":memory:");
         manager.groups.at( 1).name = "B";
         manager.groups.at( 1).queuebase.emplace( ":memory:");


         EXPECT_NO_THROW( { queue::unittest::validate( manager);});
      }

      TEST( casual_configuration_queue, validate__queue_has_to_have_unique_name)
      {
         common::unittest::Trace trace;

         queue::Manager manager;
         manager.groups.resize( 1);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase.emplace( "X");
         manager.groups.at( 0).queues.resize( 2);
         manager.groups.at( 0).queues.at( 0).name = "a";
         manager.groups.at( 0).queues.at( 1).name = "a";

         EXPECT_CODE(
         { 
            queue::unittest::validate( manager);
         }, common::code::casual::invalid_configuration);
      }

      TEST( casual_configuration_queue, validate__queue_has_to_have_unique_name_regardless_of_group)
      {
         common::unittest::Trace trace;

         queue::Manager manager;
         manager.groups.resize( 2);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase.emplace( "X");
         manager.groups.at( 0).queues.resize( 1);
         manager.groups.at( 0).queues.at( 0).name = "a";

         manager.groups.at( 1).name = "B";
         manager.groups.at( 1).queuebase.emplace( "Y");
         manager.groups.at( 1).queues.resize( 1);
         manager.groups.at( 1).queues.at( 0).name = "a";

         EXPECT_CODE(
         { 
            queue::unittest::validate( manager);
         }, common::code::casual::invalid_configuration);
      }

      TEST( casual_configuration_queue, default_values__retries)
      {
         common::unittest::Trace trace;

         queue::Manager manager;
         manager.manager_default.queue.retries = 42;
         manager.groups.resize( 1);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase.emplace( "X");
         manager.groups.at( 0).queues.resize( 1);
         manager.groups.at( 0).queues.at( 0).name = "a";

         queue::unittest::default_values( manager);
         ASSERT_TRUE( manager.groups.at( 0).queues.at( 0).retries.has_value()) << CASUAL_NAMED_VALUE( manager);
         EXPECT_TRUE( manager.groups.at( 0).queues.at( 0).retries.value() == 42) << CASUAL_NAMED_VALUE( manager);
      }


   } // config

} // casual
