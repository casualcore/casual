//!
//! test_domain.cpp
//!
//! Created on: Aug 3, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "config/domain.h"
#include "sf/log.h"

// TODO Temp
#include "../../../../serviceframework/include/sf/archive/json.h"
#include <json-c/json.h>

namespace casual
{
   class casual_configuration_domain : public ::testing::TestWithParam< const char*>
   {
   };


   INSTANTIATE_TEST_CASE_P( protocol,
         casual_configuration_domain,
      ::testing::Values("domain.yaml", "domain.json"));

	TEST_P( casual_configuration_domain, load_config)
	{

	   auto domain = config::domain::get( GetParam());

	   //
	   //
	   /*
	   json_object* root = nullptr;
	   sf::archive::json::Writer writer( root);
	   writer << CASUAL_MAKE_NVP( domain);
	   std::cerr << json_object_to_json_string( root);
	   */

	   EXPECT_TRUE( domain.name == "domain1") << "nane: " << domain.name;
	   EXPECT_TRUE( domain.groups.size() == 5) << "size: " << domain.groups.size();
	}

	TEST_P( casual_configuration_domain, read_defaul)
   {
      auto domain = config::domain::get( GetParam());

      //sf::archive::logger::Writer debug;
      //debug << CASUAL_MAKE_NVP( domain);

      ASSERT_TRUE( domain.servers.size() == 4) << "size: " << domain.servers.size();
      EXPECT_TRUE( domain.casual_default.server.instances == "2");
      EXPECT_TRUE( domain.casual_default.service.timeout == "90");

   }


   TEST_P( casual_configuration_domain, read_servers)
   {
      auto domain = config::domain::get( GetParam());

      ASSERT_TRUE( domain.servers.size() == 4) << "size: " << domain.servers.size();
      EXPECT_TRUE( domain.servers.at( 0).instances == "1");

      EXPECT_TRUE( domain.servers.at( 3).instances == "10");
      EXPECT_TRUE( domain.servers.at( 3).memberships.size() == 2);

   }

   TEST_P( casual_configuration_domain, read_transactionmanager)
   {
      auto domain = config::domain::get( GetParam());

      EXPECT_TRUE( domain.transactionmanager.database == "transaction-manager.db");

   }

   TEST_P( casual_configuration_domain, read_complement)
   {
      auto domain = config::domain::get( GetParam());

      //sf::archive::logger::Writer debug;
      //debug << CASUAL_MAKE_NVP( domain);

      ASSERT_TRUE( domain.servers.size() == 4) << "size: " << domain.servers.size();
      EXPECT_TRUE( domain.casual_default.server.instances == "2");
      EXPECT_TRUE( domain.casual_default.service.timeout == "90");

   }


} // casual
