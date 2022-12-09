//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "administration/unittest/cli/command.h"

#include "gateway/unittest/utility.h"

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
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager"
        memberships: [ queue]
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

      TEST( cli_discovery, domain_A_B__discover_service_echo_in_B__expect_found)
      {
         common::unittest::Trace trace;
         
         auto b = local::domain( R"(
domain:
   name: B
   servers:
      -  alias: example-server
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         instances: 1
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)");


         auto a = local::domain( R"(
domain:
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)");


         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         auto output = administration::unittest::cli::command::execute( "casual discovery --services casual/example/domain/echo/B --porcelain true").string();

         constexpr auto expected = R"(casual/example/domain/echo/B|1
)";

         EXPECT_TRUE( output == expected) << "output:  " << output << "expected: " << expected;
      }

      TEST( cli_discovery, domain_A_B__discover_queue_b1_in_B__expect_found)
      {
         common::unittest::Trace trace;
         
         auto b = local::domain( R"(
domain:
   name: B
   queue:
      groups:
         -  alias: B
            queues:
               -  name: b1
               -  name: b2
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)");


         auto a = local::domain( R"(
domain:
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)");


         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         auto output = administration::unittest::cli::command::execute( "casual discovery --queues b1 --porcelain true").string();

         constexpr auto expected = R"(b1
)";

         EXPECT_TRUE( output == expected) << "output:  " << output << "expected: " << expected;
      }


   } // administration
} // casual