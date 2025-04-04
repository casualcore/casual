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

   } // gateway  
} // casual