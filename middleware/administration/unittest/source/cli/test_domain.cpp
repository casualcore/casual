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

            auto execute_get_lines( auto command)
            {
               auto capture = administration::unittest::cli::command::execute( command);
               EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture);

               return string::split( capture.standard.out, '\n');
            };

         } // <unnamed>      

      } // local

      TEST( cli_domain, list_servers_executables)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   groups:
      -  name: A
         dependencies: [ user]
      -  name: B
         enabled: false
         dependencies: [ B]      
   servers:
      -  alias: a
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ A]
         instances: 2
         restart: true
      -  alias: b
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ B]
         instances: 2
      -  alias: c
         path: "/non/existent/path"
         memberships: [ A]
         instances: 2
   executables:
      -  alias: t
         path: sleep
         arguments: [ 60]
         memberships: [ A]
         instances: 0
         restart: true
      -  alias: x
         path: sleep
         instances: 2
         arguments: [ 60]
         memberships: [ A]
      -  alias: y
         instances: 2
         path: sleep
         arguments: [ 60]
         memberships: [ B]
      -  alias: z
         path: "/non/existent/path"
         memberships: [ A]
         instances: 2

)");
         {

/*
alias                       CI  I  state     restart  #r  path                                                                              
--------------------------  --  -  --------  -------  --  ----------------------------------------------------------------------------------
a                            2  2  enabled      true   0  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"  
b                            2  0  disabled    false   0  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"  
c                            2  0  error       false   0  "/non/existent/path"                                                              
casual-domain-discovery      1  1  enabled      true   0  "/Users/lazan/git/casual/1.7/casual/middleware/domain/bin/casual-domain-discovery"
casual-domain-manager        1  1  enabled     false   0  "casual-domain-manager"                                                           
casual-gateway-manager       1  1  enabled     false   0  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"        
casual-service-manager       1  1  enabled     false   0  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"        
casual-transaction-manager   1  1  enabled     false   0  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
*/
            auto lines = local::execute_get_lines(  "casual --header false --color false domain --list-servers");

            auto a = string::adjacent::split( lines.at( 0), ' ');
            EXPECT_TRUE( a.at( 0) == "a");
            EXPECT_TRUE( a.at( 1) == "2");
            EXPECT_TRUE( a.at( 2) == "2");
            EXPECT_TRUE( a.at( 3) == "enabled");
            EXPECT_TRUE( a.at( 4) == "true");
            EXPECT_TRUE( a.at( 5) == "0");

            auto b = string::adjacent::split( lines.at( 1), ' ');
            EXPECT_TRUE( b.at( 0) == "b");
            EXPECT_TRUE( b.at( 1) == "2");
            EXPECT_TRUE( b.at( 2) == "0");
            EXPECT_TRUE( b.at( 3) == "disabled");
            EXPECT_TRUE( b.at( 4) == "false");
            EXPECT_TRUE( b.at( 5) == "0");

            auto c = string::adjacent::split( lines.at( 2), ' ');
            EXPECT_TRUE( c.at( 0) == "c");
            EXPECT_TRUE( c.at( 1) == "2");
            EXPECT_TRUE( c.at( 2) == "0");
            EXPECT_TRUE( c.at( 3) == "error");
            EXPECT_TRUE( c.at( 4) == "false");
            EXPECT_TRUE( c.at( 5) == "0");

         }

         {

/*
alias  CI  I  state     restart  #r  path                
-----  --  -  --------  -------  --  --------------------
t       0  0  enabled      true   0  "sleep"             
x       2  2  enabled     false   0  "sleep"             
y       2  0  disabled    false   0  "sleep"             
z       2  0  error       false   0  "/non/existent/path"
*/
            auto lines = local::execute_get_lines( "casual --header false --color false domain --list-executables");

            auto t = string::adjacent::split( lines.at( 0), ' ');
            EXPECT_TRUE( t.at( 0) == "t");
            EXPECT_TRUE( t.at( 1) == "0");
            EXPECT_TRUE( t.at( 2) == "0");
            EXPECT_TRUE( t.at( 3) == "enabled");
            EXPECT_TRUE( t.at( 4) == "true");
            EXPECT_TRUE( t.at( 5) == "0");

            auto x = string::adjacent::split( lines.at( 1), ' ');
            EXPECT_TRUE( x.at( 0) == "x");
            EXPECT_TRUE( x.at( 1) == "2");
            EXPECT_TRUE( x.at( 2) == "2");
            EXPECT_TRUE( x.at( 3) == "enabled");
            EXPECT_TRUE( x.at( 4) == "false");
            EXPECT_TRUE( x.at( 5) == "0");

            auto y = string::adjacent::split( lines.at( 2), ' ');
            EXPECT_TRUE( y.at( 0) == "y");
            EXPECT_TRUE( y.at( 1) == "2");
            EXPECT_TRUE( y.at( 2) == "0");
            EXPECT_TRUE( y.at( 3) == "disabled");
            EXPECT_TRUE( y.at( 4) == "false");
            EXPECT_TRUE( y.at( 5) == "0");

            auto z = string::adjacent::split( lines.at( 3), ' ');
            EXPECT_TRUE( z.at( 0) == "z");
            EXPECT_TRUE( z.at( 1) == "2");
            EXPECT_TRUE( z.at( 2) == "0");
            EXPECT_TRUE( z.at( 3) == "error");
            EXPECT_TRUE( z.at( 4) == "false");
            EXPECT_TRUE( z.at( 5) == "0");
         }

      }

      TEST( cli_domain, scale_out_to_5__expected_configured_instances_5)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      -  alias: example-server
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         instances: 1
)");

         {
            auto capture = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep example-server | cut -d '|' -f 2");
            constexpr auto expected = R"(1
)";
            EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
         }

         administration::unittest::cli::command::execute( "casual domain -sa example-server 5");
         auto capture = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep example-server | cut -d '|' -f 2");
         
         constexpr auto expected = R"(5
)";

         EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
      }

      TEST( cli_domain, 5_scale_in_to_1__expected_configured_instances_1)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      -  alias: example-server
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         instances: 5
)");

         {
            auto capture = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep example-server | cut -d '|' -f 2");
            constexpr auto expected = R"(5
)";
            EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
         }

         administration::unittest::cli::command::execute( "casual domain -sa example-server 1");
         auto capture = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep example-server | cut -d '|' -f 2");
         
         constexpr auto expected = R"(1
)";

         EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
      }

      TEST( cli_domain, failed_instances__scale_out_to_5__expected_configured_instances_5)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      - path: non-existent-path
        instances: 1
)");

         administration::unittest::cli::command::execute( "casual domain -sa non-existent-path 5");
         auto capture = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep non-existent-path | cut -d '|' -f 2");
         
         constexpr auto expected = R"(5
)";

         EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
      }

      TEST( cli_domain, failed_instances__scale_in_to_1__expected_configured_instances_1)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      - path: non-existent-path
        instances: 5
)");

         {
            auto capture = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep non-existent-path | cut -d '|' -f 2");
            constexpr auto expected = R"(5
)";
            EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
         }

         administration::unittest::cli::command::execute( "casual domain -sa non-existent-path 1");
         auto capture = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep non-existent-path | cut -d '|' -f 2");
         
         constexpr auto expected = R"(1
)";

         EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
      }

      TEST( cli_domain, running_instances__scale_out_to_5__expected_configured_instances_5)
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
         auto capture = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep echo-server | cut -d '|' -f 2");
         
         constexpr auto expected = R"(5
)";

         EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture);
      }

      TEST( cli_domain, running_instances__scale_in_to_1__expected_configured_instances_1)
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
         auto capture = administration::unittest::cli::command::execute( "casual domain -ls --porcelain true | grep echo-server | cut -d '|' -f 2");
         
         constexpr auto expected = R"(1
)";

         EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
      }

      TEST( cli_domain, information__created_exists_and_contains_current_date)
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

         auto capture = administration::unittest::cli::command::execute( "casual domain --information --porcelain true | grep domain.manager.created | cut -d '|' -f 2 | grep $(date +%Y-%m-%d)");

         EXPECT_TRUE( ! capture.standard.out.empty()) << CASUAL_NAMED_VALUE( capture);
      }

      TEST( cli_domain, information__missing_instances_server)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   servers:
      - alias: unbootable
        path: "i-do-not-exist"
        memberships: [ user]
        instances: 2
)");

         auto capture = administration::unittest::cli::command::execute( "casual domain --information --porcelain true | grep domain.manager.server.instances.missing | cut -d '|' -f 2");

         constexpr auto expected = R"(2
)";

         EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
      }

      TEST( cli_domain, information__missing_instances_executable)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   executables:
      - alias: unbootable
        path: "i-do-not-exist"
        memberships: [ user]
        instances: 2
)");

         auto capture = administration::unittest::cli::command::execute( "casual domain --information --porcelain true | grep domain.manager.executable.instances.missing | cut -d '|' -f 2");

         constexpr auto expected = R"(2
)";

         EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture) << "\nexpected: " << expected;
      }

      TEST( cli_domain, environment__set_and_unset)
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

         administration::unittest::cli::command::execute( "casual domain --set-environment test 123");
         auto capture = administration::unittest::cli::command::execute( "casual domain --state");

         EXPECT_TRUE( capture.standard.out.find("test=123") != std::string::npos);

         administration::unittest::cli::command::execute( "casual domain --unset-environment test");

         capture = administration::unittest::cli::command::execute( "casual domain --state");

         EXPECT_TRUE( capture.standard.out.find("test=123") == std::string::npos);
      }

   } // administration
} // casual