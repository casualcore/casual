//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/xatmi.h"
#include "domain/unittest/manager.h"

namespace casual
{
   using namespace common;

   namespace example
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
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
        arguments: [ --work, 10ms]
)";
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }

            template< typename A>
            auto timed_action(A&& action)
            {
               auto start = platform::time::clock::type::now();
               action();
               return platform::time::clock::type::now() - start;
            }

            auto call_work = []() {
               auto buffer = tpalloc( X_OCTET, nullptr, 128);
               auto len = tptypes( buffer, nullptr, nullptr);
               tpcall( "casual/example/work", buffer, 128, &buffer, &len, 0);
               tpfree( buffer);
            };

         } // <unnamed> 

      } // local


      TEST( example_server, work_10ms__expect_at_least_10ms_elapsed_time)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain();

         auto elapsed_time = local::timed_action(local::call_work);

         EXPECT_TRUE( elapsed_time > std::chrono::milliseconds{ 10});
      }

   } // example

} // casual