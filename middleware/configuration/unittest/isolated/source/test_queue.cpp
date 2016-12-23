//!
//! casual
//!

#include <gtest/gtest.h>


#include "config/queue.h"
#include "config/domain.h"

#include "common/exception.h"
#include "common/file.h"


#include "sf/log.h"


namespace casual
{
   namespace config
   {
      namespace local
      {
         namespace
         {
            std::string get_testfile_path( const std::string& name)
            {
               return common::directory::name::base( __FILE__) + "/../" + name;
            }
         } // <unnamed>
      } // local

      TEST( casual_configuration_queue, expect_two_groups)
      {
         auto domain = config::domain::get( { local::get_testfile_path( "queue.yaml")});

         EXPECT_TRUE( domain.queue.groups.size() == 2) << CASUAL_MAKE_NVP( domain);

      }


      TEST( casual_configuration_queue, expect_4_queues_in_group_one)
      {
         auto domain = config::domain::get( { local::get_testfile_path( "queue.yaml")});

         EXPECT_TRUE( domain.queue.groups.at( 0).name == "someGroup");
         EXPECT_TRUE( domain.queue.groups.at( 0).queuebase == "some-group.qb") <<  domain.queue.groups.at( 0).queuebase;
         ASSERT_TRUE( domain.queue.groups.at( 0).queues.size() == 4) << CASUAL_MAKE_NVP( domain);
         {
            auto& queue = domain.queue.groups.at( 0).queues.at( 0);
            EXPECT_TRUE( queue.name == "queue1");
            EXPECT_TRUE( queue.retries == "3");
         }

         {
            auto& queue = domain.queue.groups.at( 0).queues.at( 3);
            EXPECT_TRUE( queue.name == "queue4");
            EXPECT_TRUE( queue.retries == "42");
         }


      }


      TEST( casual_configuration_queue, validate__group_has_to_have_a_name)
      {
         queue::Manager manager;
         manager.groups.resize( 1);

         EXPECT_THROW( { queue::unittest::validate( manager);}, common::exception::invalid::Configuration);
      }


      TEST( casual_configuration_queue, validate__group_has_to_have_unique_name)
      {
         queue::Manager manager;
         manager.groups.resize( 2);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase = "X";
         manager.groups.at( 1).name = "A";
         manager.groups.at( 1).queuebase = "Z";


         EXPECT_THROW( { queue::unittest::validate( manager);}, common::exception::invalid::Configuration);
      }

      TEST( casual_configuration_queue, validate__group_has_to_have_unique_queuebase)
      {
         queue::Manager manager;
         manager.groups.resize( 2);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase = "X";
         manager.groups.at( 1).name = "B";
         manager.groups.at( 1).queuebase = "X";


         EXPECT_THROW( { queue::unittest::validate( manager);}, common::exception::invalid::Configuration);
      }

      TEST( casual_configuration_queue, validate__queue_has_to_have_unique_name)
      {
         queue::Manager manager;
         manager.groups.resize( 1);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase = "X";
         manager.groups.at( 0).queues.resize( 2);
         manager.groups.at( 0).queues.at( 0).name = "a";
         manager.groups.at( 0).queues.at( 1).name = "a";


         EXPECT_THROW( { queue::unittest::validate( manager);}, common::exception::invalid::Configuration);
      }

      TEST( casual_configuration_queue, validate__queue_has_to_have_unique_name_regardless_of_group)
      {
         queue::Manager manager;
         manager.groups.resize( 2);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase = "X";
         manager.groups.at( 0).queues.resize( 1);
         manager.groups.at( 0).queues.at( 0).name = "a";

         manager.groups.at( 1).name = "B";
         manager.groups.at( 1).queuebase = "Y";
         manager.groups.at( 1).queues.resize( 1);
         manager.groups.at( 1).queues.at( 0).name = "a";

         EXPECT_THROW( { queue::unittest::validate( manager);}, common::exception::invalid::Configuration);
      }

      TEST( casual_configuration_queue, default_values__retries)
      {
         queue::Manager manager;
         manager.casual_default.queue.retries = "42";
         manager.groups.resize( 1);
         manager.groups.at( 0).name = "A";
         manager.groups.at( 0).queuebase = "X";
         manager.groups.at( 0).queues.resize( 1);
         manager.groups.at( 0).queues.at( 0).name = "a";

         queue::unittest::default_values( manager);
         EXPECT_TRUE( manager.groups.at( 0).queues.at( 0).retries == "42") << manager.groups.at( 0).queues.at( 0).retries;
      }


   } // config

} // casual
