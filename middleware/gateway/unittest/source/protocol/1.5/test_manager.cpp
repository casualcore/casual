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

         // we should get a send v1_5 from the server
         {
            auto message = communication::device::receive< common::message::conversation::v1_5::callee::Send>( device);
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
         const auto parent_service = std::string{ "parent-service"};
         const auto parent_span = std::string{ "parent-span"};
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

      TEST( gateway_protocol_1_5_manager, outbound_conversation_connect__no_headers)
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


         // send discovery connect directly to outbound
         const auto origin_request = []()
         {
            common::message::conversation::connect::callee::Request request{ common::process::handle()};
            request.duplex = decltype( request.duplex)::send;
            request.execution = common::strong::execution::id::generate();
            request.service.name = "a";
            request.service.requested = "b";
            request.parent.service = "q";
            request.parent.span = common::strong::execution::span::id::generate();
            request.deadline.remaining = std::chrono::seconds{ 42};
            request.trid = common::transaction::id::create();
            request.pending = std::chrono::milliseconds{ 7};
            request.buffer = common::buffer::Payload{
               .type = "X_OCTET/",
               .data = common::unittest::random::binary( 10),
               .header = casual::header::transform({
                  { "a:foo"},
                  { "b:bar"},
                  { "c:baz"}
               })};
            return request;
         }();
         

         auto correlation = common::communication::device::blocking::send( outbound_ipc, origin_request);
         

         // receive 1.5 request from outbound
         {
            auto message = communication::device::receive< common::message::conversation::connect::v1_5::callee::Request>( device, correlation);
            EXPECT_TRUE( message.correlation == correlation) << CASUAL_NAMED_VALUE( message.correlation);
            EXPECT_TRUE( message.execution == origin_request.execution) << CASUAL_NAMED_VALUE( message.execution);
            // only service.name is propagated between domains.
            EXPECT_TRUE( message.service.name == origin_request.service.name) << CASUAL_NAMED_VALUE( message.service) << " vs " << CASUAL_NAMED_VALUE( origin_request.service);
            EXPECT_TRUE( message.parent.service == origin_request.parent.service) << CASUAL_NAMED_VALUE( message.parent.service);
            // there's a new span created over the outbound.
            EXPECT_TRUE( message.parent.span);
            EXPECT_TRUE( message.parent.span != origin_request.parent.span) << CASUAL_NAMED_VALUE( message.parent.span) << " vs " << CASUAL_NAMED_VALUE( origin_request.parent.span);
            ASSERT_TRUE( message.deadline.remaining);
            EXPECT_TRUE( message.deadline == origin_request.deadline) << CASUAL_NAMED_VALUE( message.deadline);
            EXPECT_TRUE( message.trid == origin_request.trid) << CASUAL_NAMED_VALUE( message.trid);
            // pending is not propagated between domains.
            // EXPECT_TRUE( message.pending == origin_request.pending) << CASUAL_NAMED_VALUE( message.pending);
            EXPECT_TRUE( message.buffer.data == origin_request.buffer.data) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.buffer.type == origin_request.buffer.type) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.duplex == origin_request.duplex) << CASUAL_NAMED_VALUE( message.duplex);

            // send connect reply
            auto reply = common::message::conversation::connect::Reply{};
            reply.correlation = message.correlation;
            reply.execution = message.execution;
            reply.code.result = code::xatmi::ok;
            common::communication::device::blocking::send( device, reply);
         }

         // receive reply from outbound
         {
            auto reply = common::communication::ipc::receive< common::message::conversation::connect::Reply>( correlation);
            EXPECT_TRUE( reply.code.result == code::xatmi::ok) << CASUAL_NAMED_VALUE( reply.code);
         }

      }

      TEST( gateway_protocol_1_5_manager, outbound_conversation_send__no_headers)
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

         const auto origin_send = []()
         {
            common::message::conversation::callee::Send message;
            message.execution = common::strong::execution::id::generate();
            message.duplex = decltype( message.duplex)::receive;
            message.transaction_state = decltype( message.transaction_state)::ok;
            message.code = common::service::Code{ common::code::xatmi::ok, 42L};
            message.buffer.type = "X_OCTET/";
            message.buffer.data = common::unittest::random::binary( 13);
            return message;
         }();

         auto send_correlation = common::communication::device::blocking::send( outbound_ipc, origin_send);

         {
            auto message = communication::device::receive< common::message::conversation::v1_5::callee::Send>( device, send_correlation);
            EXPECT_TRUE( message.correlation == send_correlation) << CASUAL_NAMED_VALUE( message.correlation);
            EXPECT_TRUE( message.execution == origin_send.execution) << CASUAL_NAMED_VALUE( message.execution);
            EXPECT_TRUE( message.duplex == decltype( message.duplex)::receive) << CASUAL_NAMED_VALUE( message.duplex);
            EXPECT_TRUE( message.transaction_state == origin_send.transaction_state) << CASUAL_NAMED_VALUE( message.transaction_state);
            // code.result should be absent in non-terminated send messages.
            EXPECT_TRUE( message.code.result == decltype( message.code.result)::absent) << CASUAL_NAMED_VALUE( message.code);
            EXPECT_TRUE( message.code.user == origin_send.code.user) << CASUAL_NAMED_VALUE( message.code);
            EXPECT_TRUE( message.buffer.type == origin_send.buffer.type) << CASUAL_NAMED_VALUE( message.buffer.type);
            EXPECT_TRUE( message.buffer.data == origin_send.buffer.data) << CASUAL_NAMED_VALUE( message.buffer.data.size());
         }
      }

      TEST( gateway_protocol_1_5_manager, outbound_conversation_send_duplex_terminated)
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

         const auto origin_send = []()
         {
            common::message::conversation::callee::Send message;
            message.execution = common::strong::execution::id::generate();
            message.duplex = decltype( message.duplex)::terminated;
            message.transaction_state = decltype( message.transaction_state)::ok;
            message.code = common::service::Code{ common::code::xatmi::ok, 42L};
            message.buffer.type = "X_OCTET/";
            message.buffer.data = common::unittest::random::binary( 13);
            return message;
         }();

         auto send_correlation = common::communication::device::blocking::send( outbound_ipc, origin_send);

         {
            auto message = communication::device::receive< common::message::conversation::v1_5::callee::Send>( device, send_correlation);
            EXPECT_TRUE( message.correlation == send_correlation) << CASUAL_NAMED_VALUE( message.correlation);
            EXPECT_TRUE( message.execution == origin_send.execution) << CASUAL_NAMED_VALUE( message.execution);
            EXPECT_TRUE( message.duplex == decltype( message.duplex){}) << CASUAL_NAMED_VALUE( message.duplex);
            EXPECT_TRUE( message.transaction_state == origin_send.transaction_state) << CASUAL_NAMED_VALUE( message.transaction_state);
            // when duplex is terminated, the code.result should be set to NOT absent, in this case ok.
            EXPECT_TRUE( message.code.result == decltype( message.code.result)::ok) << CASUAL_NAMED_VALUE( message.code);
            EXPECT_TRUE( message.buffer.type == origin_send.buffer.type) << CASUAL_NAMED_VALUE( message.buffer.type);
            EXPECT_TRUE( message.buffer.data == origin_send.buffer.data) << CASUAL_NAMED_VALUE( message.buffer.data.size());
         }

      }

   } // gateway  
} // casual
