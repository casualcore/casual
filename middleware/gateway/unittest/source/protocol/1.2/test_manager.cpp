//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "gateway/unittest/utility.h"
#include "gateway/message.h"
#include "gateway/message/protocol.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/discover.h"
#include "domain/unittest/utility.h"

#include "transaction/unittest/utility.h"

#include "common/message/service.h"
#include "common/communication/instance.h"
#include "common/communication/tcp.h"

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
      -  path: bin/casual-gateway-manager
         memberships: [ gateway]
         environment:
            variables:
               -  key: CASUAL_INTERNAL_GATEWAY_PROTOCOL_VERSION
                  value: 1002
      
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
      -  path: bin/casual-gateway-manager
         memberships: [ gateway]
         environment:
            variables:
               -  key: CASUAL_INTERNAL_GATEWAY_PROTOCOL_VERSION
                  value: 1002

      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
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
      -  path: bin/casual-gateway-manager
         memberships: [ gateway]
         environment:
            variables:
               -  key: CASUAL_INTERNAL_GATEWAY_PROTOCOL_VERSION
                  value: 1002

      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
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


      TEST( gateway_protocol_1_2_manager, B_inbound___connect_as_v1_2__service_call)
      {

         auto b = local::domain( R"(
domain:
   name: B
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");

        
         auto device = unittest::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_2);
         EXPECT_TRUE( device.connector().socket());

         {
            common::message::service::call::v1_2::callee::Request request;
            request.service.name = "casual/example/echo";
            request.correlation = strong::correlation::id::generate();
            request.buffer.type = common::buffer::type::binary;
            request.buffer.data = unittest::random::binary( 50);
         
            auto reply = communication::device::call( device, request, device);

            EXPECT_TRUE( reply.code.result == code::xatmi::ok);
            EXPECT_TRUE( reply.buffer.data == request.buffer.data);
         }
      }

      TEST( gateway_protocol_1_2_manager, B_inbound___connect_as_v1_2__service_call_in_transaction)
      {

         auto b = local::domain( R"(
domain:
   name: B
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");

        
         auto device = unittest::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_2);
         EXPECT_TRUE( device.connector().socket());

         const auto trid = common::transaction::id::create();

         {
            const auto binary = unittest::random::binary( 1000);
            
            common::message::service::call::v1_2::callee::Request request;
            request.service.name = "casual/example/echo";
            request.trid = trid;
            request.correlation = strong::correlation::id::generate();
            request.buffer.type = common::buffer::type::binary;
            request.buffer.data = binary;
         
            auto reply = communication::device::call( device, request, device);

            EXPECT_TRUE( reply.code.result == code::xatmi::ok);
            EXPECT_TRUE( reply.transaction.trid);
            EXPECT_TRUE( reply.transaction.trid == trid);
            EXPECT_TRUE( reply.buffer.data == binary);
         }

         // for good measure
         EXPECT_TRUE( casual::transaction::unittest::commit( trid) == code::tx::ok);
      }

      TEST( gateway_protocol_1_2_manager, B_inbound___connect_as_v1_2__service_call_non_existent_service)
      {

         auto b = local::domain( R"(
domain:
   name: B
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");

        
         auto device = unittest::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_2);
         EXPECT_TRUE( device.connector().socket());

         const auto trid = common::transaction::id::create();

         {
            common::message::service::call::v1_2::callee::Request request;
            request.service.name = "non/existing/service";
            request.buffer.type = common::buffer::type::binary;
            request.buffer.data = unittest::random::binary( 50);
            request.trid = trid;
         
            auto reply = communication::device::call( device, request, device);

            EXPECT_TRUE( reply.code.result == code::xatmi::no_entry) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.transaction.trid);
            EXPECT_TRUE( reply.transaction.trid == trid);
         }

         // for good measure
         EXPECT_TRUE( casual::transaction::unittest::commit( trid) == code::tx::ok);
      }

      TEST( gateway_protocol_1_2_manager, resource_commit)
      {

         auto b = local::domain( R"(
domain:
   name: B
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");

        
         auto device = local::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_2);
         EXPECT_TRUE( device.connector().socket());

         const auto trid = common::transaction::id::create();
         
         // we do a call to involve the trid
         {
            common::message::service::call::v1_2::callee::Request request;
            request.service.name = "casual/example/echo";
            request.buffer.type = common::buffer::type::binary;
            request.buffer.data = unittest::random::binary( 50);
            request.trid = trid;
         
            auto reply = communication::device::call( device, request, device);

            EXPECT_TRUE( reply.code.result == code::xatmi::ok) << CASUAL_NAMED_VALUE( reply);
         }

         // check resoure commit message, we do a _one-phase_.
         {
            common::message::transaction::resource::commit::Request request;
            request.trid = trid;
            request.flags =  decltype( request.flags)::one_phase;     

            auto reply = communication::device::call( device, request, device);

            // NOTE: I'm not sure if read_only is the correct state here... 
            EXPECT_TRUE( reply.state == code::xa::read_only) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.trid == trid);
         }

      }

   } // gateway
   
} // casual