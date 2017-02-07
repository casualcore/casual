//!
//! casual 
//!

#include <gtest/gtest.h>
#include "common/unittest.h"
#include "configuration/domain.h"
#include "configuration/example/domain.h"

#include "sf/log.h"
#include "sf/archive/maker.h"



namespace casual
{
   namespace configuration
   {

      class configuration_gateway : public ::testing::TestWithParam< const char*>
      {
      };


      INSTANTIATE_TEST_CASE_P( protocol,
            configuration_gateway,
         ::testing::Values(".yaml", ".json", ".xml", ".ini"));


      //
      // Look at configuration/example/domain.yaml for what to expect.
      //

      TEST_P( configuration_gateway, expect_3_connections)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto gateway = domain::get( { example::temporary( example::domain(), GetParam())}).gateway;

         ASSERT_TRUE( gateway.listeners.size() == 2);
         EXPECT_TRUE( gateway.listeners.at( 0).address == "localhost:7779") << CASUAL_MAKE_NVP( gateway);
         ASSERT_TRUE( gateway.connections.size() == 3) << CASUAL_MAKE_NVP( gateway);

      }


      TEST_P( configuration_gateway, expect_3_services_for_connection_1)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto gateway = domain::get( { example::temporary( example::domain(), GetParam())}).gateway;

         ASSERT_TRUE( ! gateway.connections.empty());
         ASSERT_TRUE( gateway.connections.at( 0).type.value() == "tcp");
         ASSERT_TRUE( gateway.connections.at( 0).address.value() == "a45.domain.host.org:7779");
         ASSERT_TRUE( gateway.connections.at( 0).services.size() == 2);
      }


      TEST_P( configuration_gateway, expect_default_to_be_set)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto gateway = domain::get( { example::temporary( example::domain(), GetParam())}).gateway;

         EXPECT_TRUE( gateway.connections.at( 0).type.value() == "tcp") << CASUAL_MAKE_NVP( gateway);
         EXPECT_TRUE( gateway.connections.at( 0).restart.value() == true) << CASUAL_MAKE_NVP( gateway);

         EXPECT_TRUE( gateway.connections.at( 1).type.value() == "tcp") << CASUAL_MAKE_NVP( gateway);
         EXPECT_TRUE( gateway.connections.at( 1).restart.value() == true) << CASUAL_MAKE_NVP( gateway);

         EXPECT_TRUE( gateway.connections.at( 2).type.value() == "tcp") << CASUAL_MAKE_NVP( gateway);
         EXPECT_TRUE( gateway.connections.at( 2).restart.value() == false) << CASUAL_MAKE_NVP( gateway);

      }



   } // gateway
} // casual
