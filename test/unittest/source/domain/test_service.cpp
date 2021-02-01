//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/test/domain.h"

#include "service/manager/admin/server.h"
#include "service/manager/admin/model.h"

#include "serviceframework/service/protocol/call.h"

namespace casual
{
   using namespace common;

   namespace test::domain::service
   {
      namespace local
      {
         namespace
         {
            namespace call::state
            {
               auto service()
               {
                  serviceframework::service::protocol::binary::Call call;

                  auto reply = call( casual::service::manager::admin::service::name::state());

                  casual::service::manager::admin::model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }

            } // call::state

         } // <unnamed>
      } // local
      TEST( test_domain_service, two_server_alias__service_restriction)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: A

   groups: 
      -  name: base
      -  name: user
         dependencies: [ base]
   
   servers:
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
         
      -  alias: A
         path: "${CASUAL_REPOSITORY_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  casual/example/echo

      -  alias: B
         path: "${CASUAL_REPOSITORY_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  casual/example/sink

)";


         domain::Manager domain{ configuration};

         auto state = local::call::state::service();

         auto is_service = []( auto name)
         {
            return [name]( auto& service)
            {
               return service.name == name;
            };
         };

         {
            auto service = algorithm::find_if( state.services, is_service( "casual/example/echo"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }

         {
            auto service = algorithm::find_if( state.services, is_service( "casual/example/sink"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }
      }

   } // test::domain::service

} // casual