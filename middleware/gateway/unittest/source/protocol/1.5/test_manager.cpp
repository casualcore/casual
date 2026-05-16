//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "gateway/unittest/utility.h"
#include "gateway/message.h"

#include "common/message/transaction.h"
#include "common/communication/tcp.h"

#include "domain/unittest/manager.h"

#include "service/unittest/utility.h"

#include "queue/common/ipc/message.h"


namespace casual
{
   namespace gateway
   {
      using namespace common;

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
      - path: ${CMAKE_BINARY_DIR}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CMAKE_BINARY_DIR}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
      - path: ${CMAKE_BINARY_DIR}/middleware/queue/bin/casual-queue-manager
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
      

      TEST( gateway_protocol_1_5_manager, service_call_reply_propagate_headers)
      {
         common::unittest::Trace trace;

         auto a = local::domain( R"(
domain:
   name: A
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
      - path: ${CMAKE_BINARY_DIR}/middleware/example/server/bin/casual-example-server
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");

         casual::service::unittest::advertise( {  "a"});

        
         auto device = unittest::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_5);
         EXPECT_TRUE( device.connector().socket());

         const auto header = casual::header::transform({
            { "a:foo"},
            { "b:bar"},
            { "c:baz"}
         });

         // send call
         auto correlation = [ &]()
         {
            common::message::service::call::callee::Request request;
            request.service.name = "a";
            request.buffer.data = common::unittest::random::binary( 128);
            request.buffer.type = "X_OCTET/";
            request.buffer.header = header;

            return common::communication::device::blocking::send( device, request);
         }();

         {
            auto request = communication::ipc::receive< common::message::service::call::callee::Request>( correlation);
            EXPECT_TRUE( request.buffer.header == header) << CASUAL_NAMED_VALUE( request.buffer.header);
            auto reply = common::message::reverse::type( request);
            reply.buffer = request.buffer;
            reply.buffer.header = request.buffer.header;

            communication::device::blocking::send( request.process.ipc, reply);
            casual::service::unittest::send::ack( request);
         }

         // receive from inbound
         {
            auto reply = communication::device::receive< common::message::service::call::Reply>( device);
            EXPECT_TRUE( reply.buffer.header == header) << CASUAL_NAMED_VALUE( reply.buffer.header);

         } 
      }

      TEST( gateway_protocol_1_5_manager, conversation__tpreturn_send_message_duplex__not_value_terminate)
      {
         common::unittest::Trace trace;
         
         auto a = local::domain( R"(
domain:
   name: A
   servers:
      - path: ${CMAKE_BINARY_DIR}/middleware/example/server/bin/casual-example-server
        memberships: [ user]
      - path: ${CMAKE_BINARY_DIR}/middleware/gateway/bin/casual-gateway-manager
        memberships: [ gateway]

   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");

        
         auto device = unittest::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_5);
         EXPECT_TRUE( device.connector().socket());

         const auto data = common::unittest::random::binary( 128);


         // connect
         auto connect_reply = [ &]()
         {
            common::message::conversation::connect::callee::Request request;
            request.duplex = decltype( request.duplex)::send;
            request.service.name = "casual/example/echo";
            request.buffer.data = data;
            request.buffer.type = "X_OCTET/";

            return common::communication::device::call( device, request, device);
         }();

         EXPECT_TRUE( connect_reply.code.result == code::xatmi::ok) << CASUAL_NAMED_VALUE( connect_reply);

         // we should get a send from the server
         {
            auto message = communication::device::receive< common::message::conversation::callee::Send>( device);
            EXPECT_TRUE( message.buffer.data == data) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.duplex == decltype( message.duplex)::send) << CASUAL_NAMED_VALUE( message.duplex);

            // we should get a xatmi code -> the code from tpreturn. This indicates that the conversation is terminated.
            // other send messages has code.result set to _absent_.
            EXPECT_TRUE( message.code.result == code::xatmi::ok) << CASUAL_NAMED_VALUE( message);

         }

      }

      TEST( gateway_protocol_1_5_manager, inbound_enqueue_dequeue__no_headers)
      {
         common::unittest::Trace trace;
         
         auto a = local::domain( R"(
domain:
   name: A
   servers:
      - path: ${CMAKE_BINARY_DIR}/middleware/gateway/bin/casual-gateway-manager
        memberships: [ gateway]
   
   queue:
      groups:
         -  queuebase: ':memory:'
            queues:
               - name: a
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
         )");


         auto device = unittest::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_5);
         EXPECT_TRUE( device.connector().socket());

         const auto payload = common::unittest::random::binary( 128);
         auto available = std::chrono::time_point_cast< std::chrono::microseconds>( common::chronology::time_point::clock::now());

         // enqueue
         {
            queue::ipc::message::group::enqueue::v1_5::Request request;
            request.name = "a";
            request.message.attributes.properties = "foo";
            request.message.attributes.available = available;
            
            request.message.payload.type = "X_OCTET/";
            request.message.payload.data = payload;
            

            auto correlation = common::communication::device::blocking::send( device, request);

            auto reply = common::communication::device::receive< queue::ipc::message::group::enqueue::Reply>( device, correlation);
            EXPECT_TRUE( reply.code == common::code::queue::ok) << CASUAL_NAMED_VALUE( reply.code);
         }

         // dequeue
         {
            queue::ipc::message::group::dequeue::Request request;
            request.name = "a";

            auto correlation = common::communication::device::blocking::send( device, request);

            auto reply = common::communication::device::receive< queue::ipc::message::group::dequeue::v1_5::Reply>( device, correlation);
            EXPECT_TRUE( reply.code == common::code::queue::ok) << CASUAL_NAMED_VALUE( reply.code);
            ASSERT_TRUE( reply.message);
            EXPECT_TRUE( reply.message->payload.data == payload) << CASUAL_NAMED_VALUE( reply.message->payload.data.size());
            EXPECT_TRUE( reply.message->attributes.properties == "foo") << CASUAL_NAMED_VALUE( reply.message->attributes.properties);
            EXPECT_TRUE( reply.message->attributes.available == available) << CASUAL_NAMED_VALUE( reply.message->attributes.available) << " vs " << CASUAL_NAMED_VALUE( available);

         }
      }

      TEST( gateway_protocol_1_5_manager, outbound_enqueue_dequeue__no_headers)
      {
         common::unittest::Trace trace;
         
         auto a = local::domain( R"(
domain:
   name: A
   servers:
      - path: ${CMAKE_BINARY_DIR}/middleware/gateway/bin/casual-gateway-manager
        memberships: [ gateway]
   
   gateway:
      reverse:
         outbound:
            groups:
               -  connections: 
                  -  address: 127.0.0.1:7010
         )");


         auto device = unittest::tcp::connect::in( "127.0.0.1:7010", message::protocol::Version::v1_5);
         EXPECT_TRUE( device.connector().socket());

         auto state = gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         // the ipc that is associated with the connection. We can use this
         // to directly interact with the outbound.
         auto outbound_ipc = state.connections.at( 0).ipc;

         const auto trid = common::transaction::id::create();
         const auto timepoint = common::chronology::time_point::clock::now();
         const auto payload = common::buffer::Payload{
            .type = "X_OCTET/",
            .data = common::unittest::random::binary( 10),
            .header = casual::header::transform({
               { "a:foo"},
               { "b:bar"},
               { "c:baz"}
            })
         }; 

         const auto id = common::uuid::make();

         common::strong::correlation::id correlation;
         
         // enqueue to outbound
         {
            queue::ipc::message::group::enqueue::Request request{ common::process::handle()};
            request.name = "a";
            request.trid = trid;
            request.message.attributes.properties = "foo";
            request.message.attributes.available = timepoint;
            request.message.attributes.reply = "b";
            request.message.payload = payload;

            correlation = common::communication::device::blocking::send( outbound_ipc, request);
         }

         // receive 1.5 request from outbound
         {
            auto request = common::communication::device::receive< queue::ipc::message::group::enqueue::v1_5::Request>( device, correlation);
            EXPECT_TRUE( request.name == "a") << CASUAL_NAMED_VALUE( request.name);
            EXPECT_TRUE( request.trid == trid) << CASUAL_NAMED_VALUE( request.trid);
            EXPECT_TRUE( request.message.attributes.properties == "foo") << CASUAL_NAMED_VALUE( request.message.attributes.properties);
            EXPECT_TRUE( request.message.attributes.available == timepoint) << CASUAL_NAMED_VALUE( request.message.attributes.available) << " vs " << CASUAL_NAMED_VALUE( timepoint);
            EXPECT_TRUE( request.message.attributes.reply == "b") << CASUAL_NAMED_VALUE( request.message.attributes.reply);
            
            auto reply = common::message::reverse::type( request);
            reply.code = common::code::queue::ok;
            reply.id = id;

            common::communication::device::blocking::send( device, reply);
         }

         // get the reply from the outbound
         {
            auto reply = common::communication::ipc::receive< queue::ipc::message::group::enqueue::Reply>( correlation);
            EXPECT_TRUE( reply.code == common::code::queue::ok) << CASUAL_NAMED_VALUE( reply.code);
            EXPECT_TRUE( reply.id == id) << CASUAL_NAMED_VALUE( reply.id);
         }

         // dequeue to outbound
         {
            queue::ipc::message::group::dequeue::Request request{ common::process::handle()};
            request.name = "a";
            request.trid = trid;

            correlation = common::communication::device::blocking::send( outbound_ipc, request);
         }

         // receive request from outbound, reply with 1.5 reply
         {
            auto request = common::communication::device::receive< queue::ipc::message::group::dequeue::Request>( device, correlation);
            EXPECT_TRUE( request.name == "a") << CASUAL_NAMED_VALUE( request.name);
            EXPECT_TRUE( request.trid == trid) << CASUAL_NAMED_VALUE( request.trid);
            
            auto reply = queue::ipc::message::group::dequeue::v1_5::Reply{};
            reply.execution = request.execution;
            reply.correlation = request.correlation;
            reply.code = common::code::queue::ok;
            reply.message = queue::ipc::message::group::dequeue::v1_5::Message{
               .id = id,
               .attributes = {
                  .properties = "foo",
                  .reply = "b",
                  .available = timepoint,
               }
            };
            reply.message->payload.type = payload.type;
            reply.message->payload.data = payload.data;

            common::communication::device::blocking::send( device, reply);
         }

         // receive reply from outbound
         {
            auto reply = common::communication::ipc::receive< queue::ipc::message::group::dequeue::Reply>( correlation);
            EXPECT_TRUE( reply.code == common::code::queue::ok) << CASUAL_NAMED_VALUE( reply.code);
            ASSERT_TRUE( reply.message);
            EXPECT_TRUE( reply.message->id == id) << CASUAL_NAMED_VALUE( reply.message->id);
            EXPECT_TRUE( reply.message->attributes.properties == "foo") << CASUAL_NAMED_VALUE( reply.message->attributes.properties);
            EXPECT_TRUE( reply.message->attributes.available == timepoint) << CASUAL_NAMED_VALUE( reply.message->attributes.available) << " vs " << CASUAL_NAMED_VALUE( timepoint);
            EXPECT_TRUE( reply.message->attributes.reply == "b") << CASUAL_NAMED_VALUE( reply.message->attributes.reply);
            EXPECT_TRUE( reply.message->payload.type == payload.type) << CASUAL_NAMED_VALUE( reply.message->payload.type);
            EXPECT_TRUE( reply.message->payload.data == payload.data) << CASUAL_NAMED_VALUE( reply.message->payload.data.size());
         }

      }
   } // gateway  
} // casual
