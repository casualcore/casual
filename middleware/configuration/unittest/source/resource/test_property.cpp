//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "configuration/resource/property.h"
#include "configuration/example/resource/property.h"
#include "configuration/example/create.h"


namespace casual
{
   namespace configuration
   {
      namespace resource
      {
         class configuration_resource_property : public ::testing::TestWithParam< const char*>
         {
         };


         INSTANTIATE_TEST_SUITE_P( protocol,
               configuration_resource_property,
            ::testing::Values(".yaml", ".json", ".xml", ".ini"));



         TEST_P( configuration_resource_property, load_configuration)
         {
            // serialize and deserialize
            auto resources = resource::property::get(
               example::create::file::temporary( example::resource::property::example(), "resources", GetParam()));

            EXPECT_TRUE( resources.size() >= 2);
         }


         TEST_P( configuration_resource_property, key__expect_db2_and_rm_mockup)
         {
            // serialize and deserialize
            auto resources = resource::property::get(
               example::create::file::temporary( example::resource::property::example(), "resources", GetParam()));

            ASSERT_TRUE( resources.size() >= 2);
            EXPECT_TRUE( resources.at( 0).key == "db2");
            EXPECT_TRUE( resources.at( 1).key == "rm-mockup");

         }

         TEST_P( configuration_resource_property, xa_struct_name__expect_db2xa_switch_static_std__and__casual_mockup_xa_switch_static)
         {
            // serialize and deserialize
            auto resources = resource::property::get(
               example::create::file::temporary( example::resource::property::example(), "resources", GetParam()));

            ASSERT_TRUE( resources.size() >= 2);
            EXPECT_TRUE( resources.at( 0).xa_struct_name == "db2xa_switch_static_std");
            EXPECT_TRUE( resources.at( 1).xa_struct_name == "casual_mockup_xa_switch_static");
         }


      } // resource
   } // configuration
} // casual
