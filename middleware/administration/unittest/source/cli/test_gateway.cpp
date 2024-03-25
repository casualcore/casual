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
      -  name: user
         dependencies: [ base]
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

      TEST( cli_gateway, list_connections_legend)
      {
         common::unittest::Trace trace;
         
         auto capture = administration::unittest::cli::command::execute( "casual gateway --legend list-connections");

         EXPECT_TRUE( capture.standard.out.size() > 30) << CASUAL_NAMED_VALUE( capture);
      }

      TEST( cli_gateway, inbound_discovery_forward__expect_state_with_connection_bound__in_star)
      {
         common::unittest::Trace trace;
         
         auto b = local::domain( R"(
domain:
   name: B
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     discovery:
                        forward: true
                  -  address: 127.0.0.1:7002
                     discovery:
                        forward: false
)");


         auto a = local::domain( R"(
domain:
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                  -  address: 127.0.0.1:7002
)");


         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         b.activate();

         // extract only the bounds, and sort it for determinism.
         auto capture = administration::unittest::cli::command::execute( "casual gateway --list-connections --porcelain true | cut -d '|' -f 3 | sort" );

         constexpr auto expected = R"(in
in*
)";

         EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
      }

   

   } // administration
} // casual