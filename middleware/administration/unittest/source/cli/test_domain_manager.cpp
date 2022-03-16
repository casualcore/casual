//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "administration/unittest/cli/command.h"

#include "common/execute.h"
#include "common/result.h"
#include "common/signal.h"
#include "common/environment.h"

#include "domain/unittest/manager.h"

namespace casual
{
   using namespace common;

   namespace administration
   {
      namespace local
      {
         namespace
         {
            namespace configuration
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
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }
         } // <unnamed>      

      } // local


      TEST( domain_manager_cli, failed_instances__scale_out_to_5__expected_configured_instances_5)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      - path: non-existent-path
        instances: 1
)");

         administration::unittest::cli::command::execute( "casual domain -sa non-existent-path 5");
         auto output = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep non-existent-path | cut -d '|' -f 2").string();
         
         constexpr auto expected = R"(5
)";

         EXPECT_TRUE( output == expected) << output << expected;
      }

      TEST( domain_manager_cli, failed_instances__scale_in_to_1__expected_configured_instances_1)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      - path: non-existent-path
        instances: 5
)");

         administration::unittest::cli::command::execute( "casual domain -sa non-existent-path 1");
         auto output = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep non-existent-path | cut -d '|' -f 2").string();
         
         constexpr auto expected = R"(1
)";

         EXPECT_TRUE( output == expected) << output << expected;
      }

      TEST( domain_manager_cli, running_instances__scale_out_to_5__expected_configured_instances_5)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      - alias: echo-server
        path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
        instances: 1
)");

         administration::unittest::cli::command::execute( "casual domain -sa echo-server 5");
         auto output = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep echo-server | cut -d '|' -f 2").string();
         
         constexpr auto expected = R"(5
)";

         EXPECT_TRUE( output == expected) << output;
      }

      TEST( domain_manager_cli, running_instances__scale_in_to_1__expected_configured_instances_1)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      - alias: echo-server
        path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
        instances: 5
)");

         administration::unittest::cli::command::execute( "casual domain -sa echo-server 1");
         auto output = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep echo-server | cut -d '|' -f 2").string();
         
         constexpr auto expected = R"(1
)";

         EXPECT_TRUE( output == expected) << output << expected;
      }

     

   } // administration
} // casual