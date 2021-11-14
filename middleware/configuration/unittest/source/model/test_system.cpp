//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "configuration/system.h"
#include "configuration/example/model.h"
#include "configuration/example/create.h"


namespace casual
{
   namespace configuration
   {
      namespace resource
      {

         TEST( configuration_system, no_file)
         {
            auto resources = system::get( "").resources;
            EXPECT_TRUE( resources.empty());
         }

         class configuration_system : public ::testing::TestWithParam< const char*>
         {
         };


         INSTANTIATE_TEST_SUITE_P( protocol,
               configuration_system,
            ::testing::Values(".yaml", ".json", ".xml", ".ini"));



         TEST_P( configuration_system, load_configuration)
         {
            // serialize and deserialize
            auto system = system::get(
               example::create::file::temporary( example::user::part::system(), GetParam()));

            EXPECT_TRUE( system.resources.size() >= 2) << CASUAL_NAMED_VALUE( system);
         }


         TEST_P( configuration_system, key__expect_db2_and_rm_mockup)
         {
            // serialize and deserialize
            auto system = system::get(
               example::create::file::temporary( example::user::part::system(), GetParam()));

            ASSERT_TRUE( system.resources.size() >= 2);
            EXPECT_TRUE( system.resources.at( 0).key == "db2");
            EXPECT_TRUE( system.resources.at( 1).key == "rm-mockup");

         }

         TEST_P( configuration_system, xa_struct_name__expect_db2xa_switch_static_std__and__casual_mockup_xa_switch_static)
         {
            // serialize and deserialize
            auto system = system::get(
               example::create::file::temporary( example::user::part::system(), GetParam()));

            ASSERT_TRUE( system.resources.size() >= 2);
            EXPECT_TRUE( system.resources.at( 0).xa_struct_name == "db2xa_switch_static_std");
            EXPECT_TRUE( system.resources.at( 1).xa_struct_name == "casual_mockup_xa_switch_static");
         }


      } // resource
   } // configuration
} // casual
