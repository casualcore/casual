//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/service/manager/api/state.h"
#include "service/manager/admin/server.h"

#include "domain/manager/unittest/process.h"


namespace casual
{
   using namespace common;
   namespace service
   {
      namespace manager
      {
         namespace api
         {
            namespace local
            {
               namespace
               {
                  
                  auto is_service = []( auto name)
                  { 
                     return [name = std::move( name)]( auto& service)
                     {
                        return service.name == name;
                     };
                  };
                  

               } // <unnamed>
            } // local

            TEST( service_manager_api, state)
            {
               common::unittest::Trace trace;

               auto then = platform::time::clock::type::now();

               constexpr auto configuration = R"(
domain:
   name: service-domain

   servers:
      - path: bin/casual-service-manager
)";

               casual::domain::manager::unittest::Process manager{ { configuration}};

               // we need to call twice since the service metric is only registred 
               // after the call.
               // -> 1 get state
               // -> 2 get state
               //   validates metric from #1 (since #2 is not part of the returned state)
               {
                  auto state = manager::api::state();
                  ASSERT_TRUE( algorithm::find_if( state.services, local::is_service( admin::service::name::state())));
               }
               auto state = manager::api::state();

               ASSERT_TRUE( ! state.services.empty());
               {
                  auto found = algorithm::find_if( state.services, local::is_service( admin::service::name::state()));
                  ASSERT_TRUE( found);
                  EXPECT_TRUE( found->category == ".admin") << "found->category: " << found->category;
                  EXPECT_TRUE( found->transaction == decltype( found->transaction)::none) << "found->transaction: " << found->transaction;
                  EXPECT_TRUE( found->metric.invoked.count == 1);
                  EXPECT_TRUE( found->metric.last > then) << CASUAL_NAMED_VALUE( found->metric.last);
                  EXPECT_TRUE( found->metric.last < platform::time::clock::type::now());
                  {
                     ASSERT_TRUE( found->instances.sequential.size() == 1);
                     EXPECT_TRUE( found->instances.sequential.at( 0).pid > 0);
                     EXPECT_TRUE( found->instances.concurrent.empty());
                  }
               }

            }

         } // api
      } // manager
   } // service
} // casual