//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "domain/unittest/manager.h"

#include "gateway/unittest/utility.h"

#include "casual/xatmi.h"

namespace casual
{
   using namespace common;

   namespace test
   {

      namespace local
      {
         namespace
         {
            auto allocate( platform::size::type size = 128)
            {
               auto buffer = tpalloc( X_OCTET, nullptr, size);
               unittest::random::range( range::make( buffer, size));
               return buffer;
            }
            
         } // <unnamed>
      } // local
      
      TEST( test_gateway_reverse, call_service__from_A_to_B___expect_discovery)
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
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
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
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
   gateway:
      reverse:
         outbound:
            groups:
               -  connections:
                  -  address: 127.0.0.1:6669
)";

         auto a = casual::domain::unittest::manager( A);
         auto b = casual::domain::unittest::manager( B); // will be the 'active' domain
         
         // make sure we're connected
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "A"));


         {
            auto buffer = local::allocate( 128);
            auto len = tptypes( buffer, nullptr, nullptr);

            EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

            tpfree( buffer);
         }
         
      }

   } // test
} // casual
