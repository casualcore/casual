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

#include "queue/api/queue.h"

#include "casual/xatmi.h"

namespace casual
{
   using namespace common;

   namespace test::domain
   {

      
      TEST( test_domain_gateway, queue_service_forward___from_A_to_B__expect_discovery)
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
      inbounds:
         - connections: 
            - address: 127.0.0.1:6669
)";

         constexpr auto B = R"(
domain: 
   name: B

   groups: 
      -  name: base
      -  name: queue
         dependencies: [ base]
      -  name: gateway
         dependencies: [ queue]
   
   servers:
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/queue/bin/casual-queue-manager"
         memberships: [ queue]
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/gateway/bin/casual-gateway-manager"
         memberships: [ gateway]
   
   queue:
      groups:
         -  alias: a
            queuebase: ':memory:'
            queues:
               -  name: a1
               -  name: a2
      forward:
         groups:
            -  services:
                  -  source: a1
                     instances: 1
                     target:
                        service: casual/example/echo
                     reply:
                        queue: a2
   
   gateway:
      outbounds:
         - connections:
            - address: 127.0.0.1:6669
)";

      
         // startup the domain that will try to do stuff
         domain::Manager b{ B};
         EXPECT_TRUE( communication::instance::ping( b.handle().ipc) == b.handle());

         auto payload = unittest::random::binary( 1024);
         {
            queue::Message message;
            message.payload.type = common::buffer::type::json();
            message.payload.data = payload;
            queue::enqueue( "a1", message);
         }

         // startup the domain that has the service
         domain::Manager a{ A};
         EXPECT_TRUE( communication::instance::ping( a.handle().ipc) == a.handle());

         // activate the b domain, so we can try to dequeue
         b.activate();

         {
            auto message = queue::blocking::dequeue( "a2");

            EXPECT_TRUE( message.payload.type == common::buffer::type::json());
            EXPECT_TRUE( message.payload.data == payload);
         }
         
      }


   } // test::domain
} // casual
