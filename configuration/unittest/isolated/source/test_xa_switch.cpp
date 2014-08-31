//!
//! test_xa_switch.cpp
//!
//! Created on: Aug 3, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "config/xa_switch.h"

namespace casual
{
   class casual_configuration_resources : public ::testing::TestWithParam< const char*>
      {
      };

   INSTANTIATE_TEST_CASE_P( protocol,
         casual_configuration_resources,
         ::testing::Values("resources.yaml", "resources.json"));


	TEST_P( casual_configuration_resources, load_configuration)
	{
	   auto xa_switch = config::xa::switches::get( GetParam());

	   EXPECT_TRUE( xa_switch.size() >= 2);

	}

	TEST_P( casual_configuration_resources, key__expect_db2_and_rm_mockup)
   {
      auto xa_switch = config::xa::switches::get( GetParam());

      ASSERT_TRUE( xa_switch.size() >= 2);
      EXPECT_TRUE( xa_switch.at( 0).key == "db2");
      EXPECT_TRUE( xa_switch.at( 1).key == "rm-mockup");

   }

	TEST_P( casual_configuration_resources, xa_struct_name__expect_db2xa_switch_static_std__and__casual_mockup_xa_switch_static)
   {
      auto xa_switch = config::xa::switches::get( GetParam());

      ASSERT_TRUE( xa_switch.size() >= 2);
      EXPECT_TRUE( xa_switch.at( 0).xa_struct_name == "db2xa_switch_static_std");
      EXPECT_TRUE( xa_switch.at( 1).xa_struct_name == "casual_mockup_xa_switch_static");

   }

} // casual
