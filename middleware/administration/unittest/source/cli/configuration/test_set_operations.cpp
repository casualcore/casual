//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "administration/unittest/cli/command.h"

#include "domain/unittest/manager.h"

namespace casual::administration
{
   namespace
   {
      
      
      

      namespace local
      {
         constexpr auto base = R"(
system:
   resources:
      -  key: rm-mockup
         server: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/rm-proxy-casual-mockup"
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm

domain:
   groups: 
      -  name: base
      -  name: queue
         dependencies: [ base]
      -  name: user
         dependencies: [ queue]
      -  name: gateway
         dependencies: [ user]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
)";
         
         template< typename... C>
         auto domain( C&&... configurations)
         {
            return casual::domain::unittest::manager( base, std::forward< C>( configurations)...);
         }
      }
   }

   TEST( cli_configuration_set_operations, union)
   {
      common::unittest::Trace trace;

      auto lhs = local::domain( R"(
domain:
   servers:
      - alias: Server1
      - alias: Server2
)");

      auto rhs = common::unittest::file::temporary::content(".yaml", R"(
domain:
   servers:
      - alias: Server2
      - alias: Server3
)");

      auto command = "casual domain --configuration-get | casual configuration --union " + rhs.string();

      auto union_output = administration::unittest::cli::command::execute( command).string();

      // Expect to find all 3 servers
      EXPECT_TRUE( union_output.find("Server1") != std::string::npos) <<  union_output;
      EXPECT_TRUE( union_output.find("Server2") != std::string::npos) <<  union_output;
      EXPECT_TRUE( union_output.find("Server3") != std::string::npos) <<  union_output;
   }

   TEST( cli_configuration_set_operations, intersection)
   {
      common::unittest::Trace trace;

      auto lhs = local::domain( R"(
domain:
   servers:
      - alias: Server1
      - alias: Server2
)");

      auto rhs = common::unittest::file::temporary::content(".yaml", R"(
domain:
   servers:
      - alias: Server2
      - alias: Server3
)");

      auto command = "casual domain --configuration-get | casual configuration --intersection " + rhs.string();

      auto intersection_output = administration::unittest::cli::command::execute( command).string();

      // Expect to only find Server2
      EXPECT_TRUE( intersection_output.find("Server2") != std::string::npos) <<  intersection_output;

      EXPECT_TRUE( intersection_output.find("Server1") == std::string::npos) <<  intersection_output;
      EXPECT_TRUE( intersection_output.find("Server3") == std::string::npos) <<  intersection_output;
   }

   TEST( cli_configuration_set_operations, difference)
   {
      common::unittest::Trace trace;

      auto lhs = local::domain( R"(
domain:
   servers:
      - alias: Server1
      - alias: Server2
)");

      auto rhs = common::unittest::file::temporary::content(".yaml", R"(
domain:
   servers:
      - alias: Server2
      - alias: Server3
)");

      auto command = "casual domain --configuration-get | casual configuration --difference " + rhs.string();

      auto difference_output = administration::unittest::cli::command::execute( command).string();

      // Expect to only find Server1
      EXPECT_TRUE( difference_output.find("Server1") != std::string::npos) <<  difference_output;

      EXPECT_TRUE( difference_output.find("Server2") == std::string::npos) <<  difference_output;
      EXPECT_TRUE( difference_output.find("Server3") == std::string::npos) <<  difference_output;
   }

   TEST( cli_configuration_set_operations, union__xml_from_lhs__yaml_from_rhs)
   {
      common::unittest::Trace trace;

      auto lhs = local::domain( R"(
domain:
   servers:
      - alias: Server1
      - alias: Server2
)");

      auto rhs = common::unittest::file::temporary::content(".yaml", R"(
domain:
   servers:
      - alias: Server2
      - alias: Server3
)");

      auto command = "casual domain --configuration-get xml | casual configuration --format xml --union " + rhs.string();

      auto union_output = administration::unittest::cli::command::execute( command).string();

      // Expect to find all 3 servers
      EXPECT_TRUE( union_output.find("Server1") != std::string::npos) <<  union_output;
      EXPECT_TRUE( union_output.find("Server2") != std::string::npos) <<  union_output;
      EXPECT_TRUE( union_output.find("Server3") != std::string::npos) <<  union_output;
   }
}
