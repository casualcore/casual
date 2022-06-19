//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "gateway/unittest/utility.h"
#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/discover.h"
#include "service/unittest/utility.h"

#include "serviceframework/service/protocol/call.h"

#include "common/communication/instance.h"


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
               
               static constexpr auto base = R"(
domain: 
   name: base
   groups: 
      - name: base
      - name: gateway
        dependencies: [ base]
   
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
)";

            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }


         } // unnamed
      } // local

      TEST( gateway_manager_reverse, four_inbounds_outbounds_connections_127_0_0_1__6669)
      {
         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain:
   name: B
   gateway:
      reverse:
         inbound:
            groups:
               -  connections: 
                  - address: 127.0.0.1:6669
                  - address: 127.0.0.1:6669
                  - address: 127.0.0.1:6669
                  - address: 127.0.0.1:6669

)");

         auto a = local::domain( R"(
domain:
   name: A
   gateway:
      reverse:
         outbound:
            groups:
               -  connections:
                  -  address: 127.0.0.1:6669
)");

         auto state = gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( 4));
         EXPECT_TRUE( gateway::unittest::fetch::predicate::outbound::connected( "B")( state));
      }

      TEST( gateway_manager_reverse, advertise_b__discovery__expect_to_find_b)
      {
         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain:
   name: B
   gateway:
      reverse:
         inbound:
            groups:
               -  connections: 
                  - address: 127.0.0.1:6669
)");

         casual::service::unittest::advertise( { "b"});

         auto a = local::domain( R"(
domain:
   name: A
   gateway:
      reverse:
         outbound:
            groups:
               -  connections:
                  -  address: 127.0.0.1:6669
)");

         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "B"));

         casual::domain::unittest::discover( { "b"}, {});

         // check that service has concurrent instances
         {
            auto service = casual::service::unittest::state();
            auto found = algorithm::find( service.services, "b");
            ASSERT_TRUE( found) << CASUAL_NAMED_VALUE( service);
            EXPECT_TRUE( ! found->instances.concurrent.empty()) << CASUAL_NAMED_VALUE( *found);
         }

      }

      TEST( gateway_manager_reverse, advertise_b__discovery__call_b_10_times)
      {
         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain:
   name: B
   gateway:
      reverse:
         inbound:
            groups:
               -  connections: 
                  - address: 127.0.0.1:6669
)");

         casual::service::unittest::advertise( { "b"});

         auto a = local::domain( R"(
domain:
   name: A
   gateway:
      reverse:
         outbound:
            groups:
               -  connections:
                  -  address: 127.0.0.1:6669
)");

         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "B"));

         const auto payload = common::unittest::random::binary( 512);


         // call b a few times
         auto correlations = algorithm::generate_n< 10>( [ &payload](){
            return common::unittest::service::send( "b", payload);
         });

         // we enter B and reply to to the request
         b.activate();

         for( auto& correlation : correlations)
         {
            auto request = common::unittest::service::receive< common::message::service::call::callee::Request>( communication::ipc::inbound::device(), correlation);
            casual::service::unittest::send::ack( request);

            auto reply = common::message::reverse::type( request);
            reply.buffer = std::move( request.buffer);
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // we enter A and receive the replies
         a.activate();

         for( auto& correlation : correlations)
         {
            auto reply = common::unittest::service::receive< common::message::service::call::Reply>( communication::ipc::inbound::device(), correlation);
            EXPECT_TRUE( reply.buffer.memory == payload);
         }
      }

   } // gateway
} // casual