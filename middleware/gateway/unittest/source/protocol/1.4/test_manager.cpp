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
#include "common/communication/instance.h"

#include "domain/unittest/manager.h"
#include "domain/message/discovery.h"

#include "service/lookup.h"
#include "service/unittest/utility.h"


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
      

      TEST( gateway_protocol_1_4_manager, transaction_resource_messages__unknown_trid___expect_read_only)
      {
         common::unittest::Trace trace;

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

        
         auto device = unittest::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_4);
         EXPECT_TRUE( device.connector().socket());

         auto send_resource_message = [ &device]( auto request)
         {
             // We send an unknown trid to the domain, we expect read-only back
            request.trid = common::transaction::id::create();;
            request.resource = strong::resource::id{ 1};

            auto reply = communication::device::call( device, request, device);

            EXPECT_TRUE( reply.state == code::xa::read_only) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.trid == request.trid);
            EXPECT_TRUE( reply.resource == request.resource);
         };

         send_resource_message( common::message::transaction::resource::prepare::Request{});
         send_resource_message( common::message::transaction::resource::commit::Request{});
         send_resource_message( common::message::transaction::resource::rollback::Request{}); 
      }

      TEST( gateway_protocol_1_4_manager, service_call__with_exact_trid_that_is_already_branched)
      {
         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain:
   name: B
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
                     discovery: 
                        forward: true
         )");


         casual::service::unittest::concurrent::advertise( {  "b"});

        
         auto device = unittest::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_4);
         EXPECT_TRUE( device.connector().socket());

         const auto trid = common::transaction::id::create();
         
         static constexpr auto send_v1_4_call = []( auto& device, const auto& trid)
         {
            common::message::service::call::v1_4::callee::Request request;
            request.service.name = "b";
            request.trid = trid;
            request.correlation = common::strong::correlation::id::generate();
            request.buffer.data = common::unittest::random::binary( 128);
            request.buffer.type = "X_OCTET/";

            return common::communication::device::blocking::send( device, request);
         };

         // send the first call to b with a new trid, via inbound
         auto correlation_a = send_v1_4_call( device, trid);
         
         // receive the call to a from within 'B' domain
         auto request_a = communication::ipc::receive< common::message::service::call::callee::Request>( correlation_a);
         // check that the gtrid are the same and the branch differs
         EXPECT_TRUE( request_a.trid.global() == trid.global()) << CASUAL_NAMED_VALUE( request_a.trid) << '\n' << CASUAL_NAMED_VALUE( trid);
         EXPECT_TRUE( request_a.trid.branch() != trid.branch()) << CASUAL_NAMED_VALUE( trid);

         // send another call to b with the exact same branched trid.
         auto correlation_b = send_v1_4_call( device, request_a.trid);

         // receive the call to b from within 'B' domain
         auto request_b = communication::ipc::receive< common::message::service::call::callee::Request>( correlation_b);
         EXPECT_TRUE( request_b.trid == request_a.trid) << CASUAL_NAMED_VALUE( request_b.trid);
      }

      TEST( gateway_protocol_1_4_manager, outbound_call_with_trid_request__expect_1_4__request)
      {
         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain:
   name: B
   servers:
      -  path: bin/casual-gateway-manager
         memberships: [ gateway]
   gateway:
      reverse:
         outbound:
            groups:
               -  connections: 
                     -  address: 127.0.0.1:7010
            )");

         // connect as a _mockup-domain_ to the gateway.
         auto device = unittest::tcp::connect::in( "127.0.0.1:7010", message::protocol::Version::v1_4);
         EXPECT_TRUE( device.connector().socket());

         // we wait until outbound is connected properly -> done the registration to discovery.
         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         const auto data = common::unittest::random::binary( 128);

         auto lookup = casual::service::Lookup{ "b", {}};
         
         // In our _mockup-domain_, expect discovery request to come in
         {
            auto request = communication::device::receive< casual::domain::message::discovery::Request>( device);
            EXPECT_TRUE( request.domain.name == "B");
            EXPECT_TRUE( std::ranges::contains( request.content.services, "b"));

            // reply that we have the service.
            auto reply = common::message::reverse::type( request);
            reply.content.services.push_back( { "b", {}});


            communication::device::blocking::send( device, reply);
         }

         const auto deadline_remaining = std::chrono::seconds( 10);
         const auto trid = common::transaction::id::create();

         // call the service via outbound
         {
            auto lookup_reply = casual::service::lookup::reply( std::move( lookup));
            ASSERT_TRUE( lookup_reply.state == decltype( lookup_reply.state)::idle) << CASUAL_NAMED_VALUE( lookup_reply);

            common::message::service::call::callee::Request request{ common::process::handle()};
            request.service.name = "b";
            request.flags = decltype( request.flags)::no_time;
            request.correlation = lookup_reply.correlation;
            request.buffer.data = data;
            request.buffer.type = "X_OCTET/";
            request.deadline.remaining = deadline_remaining;
            request.trid = trid;

            communication::device::blocking::send( lookup_reply.process.ipc, request);
         }

         // In our _mockup-domain_, expect v1_4 call request to come in, reply with v1_4 reply
         {
            auto request = communication::device::receive< common::message::service::call::v1_4::callee::Request>( device);
            EXPECT_TRUE( request.service.name == "b");
            EXPECT_TRUE( request.flags == decltype( request.flags)::no_time);
            EXPECT_TRUE( request.buffer.data == data);
            EXPECT_TRUE( request.buffer.type == "X_OCTET/");
            EXPECT_TRUE( request.deadline.remaining == deadline_remaining);
            EXPECT_TRUE( request.trid == trid);

            common::message::service::call::v1_4::Reply reply;
            reply.execution = request.execution;
            reply.correlation = request.correlation;
            reply.code.result = decltype( reply.code.result)::ok;
            reply.buffer = std::move( request.buffer);
            reply.transaction_state = decltype( reply.transaction_state)::ok;
            communication::device::blocking::send( device, reply);
         }

         // expect the reply from outbound
         {
            auto reply = communication::ipc::receive< common::message::service::call::Reply>();
            EXPECT_TRUE( reply.code.result == decltype( reply.code.result)::ok);
            EXPECT_TRUE( reply.buffer.data == data);
            EXPECT_TRUE( reply.buffer.type == "X_OCTET/");
            EXPECT_TRUE( reply.transaction_state == decltype( reply.transaction_state)::ok);
         }

         // transaction cleanup, for good measure. Also good to make sure the commit messages are correct.
         // send commit request to TM.
         {
            common::message::transaction::commit::Request commit{ common::process::handle()};
            commit.trid = trid;
            communication::device::blocking::send( communication::instance::outbound::transaction::manager::device(), commit);
         }

         // In our _mockup-domain_, expect resource commit request (one-phase-optimization) to come in with the same trid, reply with read-only
         {
            auto request = communication::device::receive< common::message::transaction::resource::commit::Request>( device);
            EXPECT_TRUE( request.trid == trid);

            auto reply = common::message::reverse::type( request);
            reply.state = code::xa::read_only;
            reply.trid = request.trid;
            communication::device::blocking::send( device, reply);
         }

         // expect commit reply from TM
         {
            auto reply = communication::ipc::receive< common::message::transaction::commit::Reply>();
            EXPECT_TRUE( reply.state == decltype( reply.state)::ok);
            EXPECT_TRUE( reply.trid == trid);
         }

      }

   } // gateway  
} // casual
