//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "gateway/unittest/utility.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/discover.h"
#include "domain/unittest/utility.h"

#include "queue/api/queue.h"

namespace casual
{
   using namespace common;
   namespace gateway
   {
      namespace local
      {
         namespace
         {
            namespace configuration
            {
               
               constexpr auto servers = R"(
domain: 

   groups: 
      - name: base
      - name: user
        dependencies: [ base]
      - name: gateway
        dependencies: [ user]
   
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
)";
     
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::servers, std::forward< C>( configurations)...);
            }
         } // <unnamed>
      } // local


      TEST( gateway_protocol_1_2_manager, B_inbound_1_2__A_outbound_1_3__connect)
      {

         auto b = local::domain( R"(
domain: 
   name: B
   servers:
      - path: bin/casual-gateway-manager.1.2
        memberships: [ gateway]
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");

         auto a = local::domain( R"(
domain: 
   name: A
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
   gateway:
      outbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");


         const auto state = unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         {
            auto& connection = state.connections.at( 0);
            EXPECT_TRUE( connection.protocol == decltype( connection.protocol)::v1_2);
         }
      }


      TEST( gateway_protocol_1_2_manager, B_inbound_1_2__A_outbound_1_3__enqueue_dequeue)
      {

         auto b = local::domain( R"(
domain: 
   name: B
   servers:
      - path: bin/casual-gateway-manager.1.2
        memberships: [ gateway]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
        memberships: [ base]
   queue:
      groups:
         -  alias: QB
            queuebase: ":memory:"
            queues:
               - name: b
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");

         auto a = local::domain( R"(
domain: 
   name: A
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
        memberships: [ base]
   gateway:
      outbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");


         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         const auto payload = unittest::random::binary( 1000);
         
         EXPECT_TRUE( queue::enqueue( "b", { { "binary", payload}}));

         {
            auto message = queue::dequeue( "b");
            ASSERT_TRUE( message.size() == 1);

            EXPECT_TRUE( message.at( 0).payload.data == payload);
         }
      }


      TEST( gateway_protocol_1_2_manager, B_inbound_1_3__A_outbound_1_2__enqueue_dequeue)
      {

         auto b = local::domain( R"(
domain: 
   name: B
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
        memberships: [ base]
   queue:
      groups:
         -  alias: QB
            queuebase: ":memory:"
            queues:
               - name: b
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");

         auto a = local::domain( R"(
domain: 
   name: A
   servers:
      - path: bin/casual-gateway-manager.1.2
        memberships: [ gateway]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
        memberships: [ base]
   gateway:
      outbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");


         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         const auto payload = unittest::random::binary( 1000);
         
         EXPECT_TRUE( queue::enqueue( "b", { { "binary", payload}}));

         {
            auto message = queue::dequeue( "b");
            ASSERT_TRUE( message.size() == 1);

            EXPECT_TRUE( message.at( 0).payload.data == payload);
         }
      }

   } // gateway
   
} // casual