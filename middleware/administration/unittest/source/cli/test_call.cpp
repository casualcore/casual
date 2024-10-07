//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "administration/unittest/cli/command.h"

#include "common/execute.h"
#include "common/result.h"
#include "common/signal.h"
#include "common/code/xatmi.h"
#include "common/terminal.h"
#include "common/exception/format.h"

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
         
            namespace check
            {
               template< typename C, typename... Ts>
               auto format( std::string_view output, C code)
               {
                  auto error_string = [ code]()
                  {
                     std::ostringstream out;
                     exception::format::terminal( out, exception::compose( code));
                     return std::move( out).str();
                  }();

                  return predicate::boolean( algorithm::search( output, error_string));
               }
            } // check

         } // <unnamed> 

      } // local


      TEST( cli_call, synchronous_call)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
        arguments: [ --sleep, 10ms]
)");


         auto output = administration::unittest::cli::command::execute( R"(echo "casual" | casual buffer --compose | casual call --service casual/example/echo | casual buffer --extract)").standard.out;
         EXPECT_TRUE( output == "casual\n") << output;
      }

      TEST( cli_call, synchronous_call_iterations_10)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
        arguments: [ --sleep, 10ms]
)");


         auto output = administration::unittest::cli::command::execute( R"(echo "casual" | casual buffer --compose | casual call --iterations 10 --service casual/example/echo | casual buffer --extract)").standard.out;
         constexpr auto expected = R"(casual
casual
casual
casual
casual
casual
casual
casual
casual
casual
)";

         EXPECT_TRUE( output == expected) << output;
      }

      TEST( cli_call, synchronous_call_no_entry)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
        arguments: [ --sleep, 10ms]
)");

         auto capture = administration::unittest::cli::command::execute( R"(echo "casual" | casual buffer --compose | casual call --service non.existent)");
         EXPECT_TRUE( local::check::format( capture.standard.error, code::xatmi::no_entry)) << CASUAL_NAMED_VALUE( capture);
      }

      TEST( cli_call, synchronous_call_system)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-error-server"
        memberships: [ user]
        arguments: [ --sleep, 10ms]
)");

         auto capture = administration::unittest::cli::command::execute( R"(echo "casual" | casual buffer --compose | casual call --service casual/example/error/TPESYSTEM)");
         EXPECT_TRUE( local::check::format( capture.standard.error, code::xatmi::system)) << CASUAL_NAMED_VALUE( capture);
      }

      TEST( cli_call, show_examples)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain();

         auto capture = unittest::cli::command::execute( R"(casual call --examples)");

         // don't really know how to test stuff like this. 
         EXPECT_TRUE( algorithm::search( capture.standard.out, std::string_view( "examples:"))) << CASUAL_NAMED_VALUE( capture);
         
      }

   } // administration
} // casual