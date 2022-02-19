//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/domain/manager/api/internal/transform.h"
#include "domain/manager/admin/server.h"
#include "domain/unittest/manager.h"
#include "domain/unittest/internal/call.h"

namespace casual
{
   using namespace common;
   namespace domain
   {
      namespace manager
      {
         namespace api
         {
            namespace local
            {
               namespace
               {
  
                  auto is_server = []( auto alias)
                  { 
                     return [alias = std::move( alias)]( auto& server)
                     {
                        return server.alias == alias;
                     };
                  };

                  namespace call
                  {
                     auto state()
                     {
                        // we do the 'hack' call, and transform the result.
                        return api::internal::transform::state( unittest::internal::call< admin::model::State>( admin::service::name::state));
                     }
                  }

               } // <unnamed>
            } // local

            TEST( domain_manager_api, state)
            {
               common::unittest::Trace trace;

               auto then = platform::time::clock::type::now();


               constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      instances: 1

)";

               auto manager = unittest::manager( configuration);

               auto state = local::call::state();

               auto found = algorithm::find_if( state.servers, local::is_server( "test-simple-server"));
               ASSERT_TRUE( found);
               ASSERT_TRUE( found->instances.size() == 1);
               {
                  auto& instance = found->instances.at( 0);
                  EXPECT_TRUE( instance.state == decltype( instance.state)::running);
                  EXPECT_TRUE( instance.spawnpoint > then);
                  EXPECT_TRUE( instance.spawnpoint < platform::time::clock::type::now());
               }
            }

         } // api
      } // manager
   } // domain
} // casual