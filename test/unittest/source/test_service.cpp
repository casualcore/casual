//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/unittest/manager.h"

#include "service/unittest/utility.h"



namespace casual
{
   using namespace common;

   namespace test::domain::service
   {
      namespace local
      {
         namespace
         {


            namespace configuration
            {
               constexpr auto base =  R"(
domain: 
   groups: 
      -  name: base
      -  name: user
         dependencies: [ base]
   
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
   
)";
            } // configuration

            namespace is
            {
               auto service( std::string_view name)
               {
                  return [name]( auto& service)
                  {
                     return service.name == name;
                  };
               }
                        
            } // is   

         } // <unnamed>
      } // local

      TEST( test_service, two_server_alias__service_restriction)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain: 
   name: A
   servers:         
      -  alias: A
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  casual/example/echo

      -  alias: B
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  casual/example/sink

)");

         auto state = casual::service::unittest::state();

         // check that we excluded services that doesn't match the restrictions
         EXPECT_TRUE( ! algorithm::find_if( state.services, local::is::service( "casual/example/uppercase")));


         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/echo"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }

         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/sink"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }
      }

      TEST( test_service, service_restriction_regex)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain: 
   name: A
   servers:         
      -  alias: A
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  ".*/echo$"
            -  ".*/sink$"

)");

         auto state = casual::service::unittest::state();

         // check that we excluded services that doesn't match the restrictions
         EXPECT_TRUE( ! algorithm::find_if( state.services, local::is::service( "casual/example/uppercase")));


         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/echo"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }

         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/sink"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }
      }

   } // test::domain::service

} // casual