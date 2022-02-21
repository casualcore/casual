//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/utility.h"

#include "common/communication/instance.h"

namespace casual
{
   using namespace common;

   namespace test
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
)";


            } // configuration
         } // <unnamed>
      } // local
      
      TEST( test_domain, no_configuration)
      {
         common::unittest::Trace trace;

         auto manager = casual::domain::unittest::manager();

         EXPECT_TRUE( communication::instance::ping( manager.handle().ipc) == manager.handle());
      }


      TEST( test_domain, example_server_4_instances___kill_one___expect_shutdown)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain:
   servers:
      -  alias: example
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         instances: 4
)");

         // find and kill one instance
         {
            auto state = casual::domain::unittest::fetch::until( casual::domain::unittest::fetch::predicate::alias::has::instances( "example", 4));

            auto example = algorithm::find( state.servers, "example");
            ASSERT_TRUE( example);
            signal::send( example->instances.at( 1).handle.pid, code::signal::terminate);
         }

         casual::domain::unittest::fetch::until( casual::domain::unittest::fetch::predicate::alias::has::instances( "example", 3));

      }


   } // test

} // casual
