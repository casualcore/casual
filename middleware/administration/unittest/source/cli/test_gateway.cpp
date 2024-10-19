//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "administration/unittest/cli/command.h"

#include "gateway/unittest/utility.h"

#include "domain/unittest/manager.h"

#include "common/string.h"

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

            auto execute_get_lines( auto command)
            {
               return string::split( administration::unittest::cli::command::execute( command).standard.out, '\n');
            };

         } // <unnamed>

      } // local

      TEST( cli_gateway, list_connections_legend)
      {
         common::unittest::Trace trace;
         
         auto capture = administration::unittest::cli::command::execute( "casual gateway --legend --list-connections");

         EXPECT_TRUE( capture.standard.out.size() > 30) << CASUAL_NAMED_VALUE( capture);
      }


      TEST( cli_gateway, list_connections)
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

/*
name  id                                group     bound  runlevel   P    local            peer            created                         
----  --------------------------------  --------  -----  ---------  ---  ---------------  --------------  --------------------------------
B     b2793f25142543c19a42c3818fb779f5  outbound  out    connected  1.4  127.0.0.1:59810  127.0.0.1:7001  2024-07-16T15:13:50.610546+02:00
B     b2793f25142543c19a42c3818fb779f5  outbound  out    connected  1.4  127.0.0.1:59809  127.0.0.1:7001  2024-07-16T15:13:50.610016+02:00
B     b2793f25142543c19a42c3818fb779f5  outbound  out    connected  1.4  127.0.0.1:59808  127.0.0.1:7001  2024-07-16T15:13:50.607818+02:00
*/


         const auto columns = string::adjacent::split( local::execute_get_lines( "casual --color false --header false gateway --list-connections").at( 0), ' ');

         EXPECT_TRUE( algorithm::contains( columns, "B"));
         EXPECT_TRUE( algorithm::contains( columns, "outbound"));
         EXPECT_TRUE( algorithm::contains( columns, "out"));
         EXPECT_TRUE( algorithm::contains( columns, "connected"));
         EXPECT_TRUE( algorithm::contains( columns, "1.4"));
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