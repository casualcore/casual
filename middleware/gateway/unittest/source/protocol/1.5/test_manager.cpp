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
      

      TEST( gateway_protocol_1_5_manager, service_call_reply_propagate_headers)
      {
         common::unittest::Trace trace;

         auto a = local::domain( R"(
domain:
   name: A
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

         casual::service::unittest::advertise( {  "a"});

        
         auto device = unittest::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_5);
         EXPECT_TRUE( device.connector().socket());

         const auto header = casual::header::Fields{{
            { "a", "foo"},
            { "b", "bar"},
            { "c", "baz"}
         }};

         // send call
         auto correlation = [ &]()
         {
            common::message::service::call::callee::Request request;
            request.service.name = "a";
            request.buffer.data = common::unittest::random::binary( 128);
            request.buffer.type = "X_OCTET/";
            request.header = header;

            return common::communication::device::blocking::send( device, request);
         }();

         {
            auto request = communication::ipc::receive< common::message::service::call::callee::Request>( correlation);
            EXPECT_TRUE( request.header == header) << CASUAL_NAMED_VALUE( request.header);
            auto reply = common::message::reverse::type( request);
            reply.buffer = request.buffer;
            reply.header = request.header;

            communication::device::blocking::send( request.process.ipc, reply);
            casual::service::unittest::send::ack( request);
         }

         // receive from inbound
         {
            auto reply = communication::device::receive< common::message::service::call::Reply>( device);
            EXPECT_TRUE( reply.header == header) << CASUAL_NAMED_VALUE( reply.header);

         } 
      }


   } // gateway  
} // casual
