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

         static constexpr auto send_call = []( auto& device, const auto& trid)
         {
            common::message::service::call::callee::Request request;
            request.service.name = "b";
            request.trid = trid;
            request.correlation = common::strong::correlation::id::generate();
            request.buffer.data = common::unittest::random::binary( 128);
            request.buffer.type = "X_OCTET/";

            return common::communication::device::blocking::send( device, request);
         };

         // send the first call to b with a new trid, via inbound
         auto correlation_a = send_call( device, trid);
         
         // receive the call to a from within 'B' domain
         auto request_a = communication::ipc::receive< common::message::service::call::callee::Request>( correlation_a);
         // check that the gtrid are the same and the branch differs
         EXPECT_TRUE( request_a.trid.global() == trid.global()) << CASUAL_NAMED_VALUE( request_a.trid) << '\n' << CASUAL_NAMED_VALUE( trid);
         EXPECT_TRUE( request_a.trid.branch() != trid.branch()) << CASUAL_NAMED_VALUE( trid);

         // send another call to b with the exact same branched trid.
         auto correlation_b = send_call( device, request_a.trid);

         // receive the call to b from within 'B' domain
         auto request_b = communication::ipc::receive< common::message::service::call::callee::Request>( correlation_b);
         EXPECT_TRUE( request_b.trid == request_a.trid) << CASUAL_NAMED_VALUE( request_b.trid);
      }

   } // gateway  
} // casual