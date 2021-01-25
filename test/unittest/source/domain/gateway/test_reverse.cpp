//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "casual/test/domain.h"

#include "common/communication/instance.h"
#include "serviceframework/service/protocol/call.h"

#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"

#include "casual/xatmi.h"

namespace casual
{
   using namespace common;

   namespace test::domain
   {

      namespace local
      {
         namespace
         {

            namespace call
            {
               auto state()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( gateway::manager::admin::service::name::state);

                  gateway::manager::admin::model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }
            }

            namespace state
            {
               namespace gateway
               {
                  auto call()
                  {
                     serviceframework::service::protocol::binary::Call call;
                     auto reply = call( casual::gateway::manager::admin::service::name::state);

                     casual::gateway::manager::admin::model::State result;
                     reply >> CASUAL_NAMED_VALUE( result);

                     return result;
                  }

                  template< typename P>
                  auto until( P&& predicate)
                  {
                     auto state = state::gateway::call();

                     auto count = 1000;

                     while( ! predicate( state) && count-- > 0)
                     {
                        process::sleep( std::chrono::milliseconds{ 2});
                        state = call::state();
                     }

                     return state;
                  }

               } // gateway

               
            } // state

            auto allocate( platform::size::type size = 128)
            {
               auto buffer = tpalloc( X_OCTET, nullptr, size);
               unittest::random::range( range::make( buffer, size));
               return buffer;
            }
            
         } // <unnamed>
      } // local
      
      TEST( test_domain_gateway_reverse, call_service__from_A_to_B___expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   groups: 
      - name: base
      - name: user
        dependencies: [ base]
      - name: gateway
        dependencies: [ user]
   
   servers:
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
   gateway:
      reverse:
         inbound:
            groups:
               -  connections: 
                  -  address: 127.0.0.1:6669
)";

         constexpr auto B = R"(
domain: 
   name: B

   groups: 
      - name: base
      - name: gateway
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
   gateway:
      reverse:
         outbound:
            groups:
               -  connections:
                  -  address: 127.0.0.1:6669
)";

         domain::Manager a{ A};
         domain::Manager b{ B}; // will be the 'active' domain

         EXPECT_TRUE( communication::instance::ping( a.handle().ipc) == a.handle());
         EXPECT_TRUE( communication::instance::ping( b.handle().ipc) == b.handle());

         // make sure we're connected
         {
            auto state = local::state::gateway::until( []( auto& state)
            { 
               return ! state.connections.empty() && ! state.connections[ 0].remote.name.empty();
            });

            EXPECT_TRUE( state.connections.at( 0).remote.name == "A");
         }

         {
            auto buffer = local::allocate( 128);
            auto len = tptypes( buffer, nullptr, nullptr);

            EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

            tpfree( buffer);
         }

         
      }


   } // test::domain
} // casual
