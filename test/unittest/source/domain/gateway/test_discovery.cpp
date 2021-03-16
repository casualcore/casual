//! 
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "domain/manager/unittest/process.h"

#include "common/communication/instance.h"
#include "serviceframework/service/protocol/call.h"

#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"


#include "casual/xatmi.h"

namespace casual
{
   using namespace common;

   namespace test::domain
   {

      namespace local
      {
         namespace
         {
            using Manager = casual::domain::manager::unittest::Process;

            namespace call
            {
               auto state()
               {
                  // to ensure that we use the 'current' domain
                  communication::instance::outbound::service::manager::device().connector().clear();

                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( gateway::manager::admin::service::name::state);

                  gateway::manager::admin::model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }
            }

            namespace state
            {
               namespace gateway
               {
                  auto call()
                  {
                     // to ensure that we use the 'current' domain
                     communication::instance::outbound::service::manager::device().connector().clear();

                     serviceframework::service::protocol::binary::Call call;
                     auto reply = call( casual::gateway::manager::admin::service::name::state);

                     casual::gateway::manager::admin::model::State result;
                     reply >> CASUAL_NAMED_VALUE( result);

                     return result;
                  }

                  template< typename P>
                  auto until( P&& predicate)
                  {
                     auto state = state::gateway::call();

                     auto count = 1000;

                     while( ! predicate( state) && count-- > 0)
                     {
                        process::sleep( std::chrono::milliseconds{ 2});
                        state = call::state();
                     }

                     return state;
                  }

               } // gateway

               
            } // state

            auto allocate( platform::size::type size = 128)
            {
               auto buffer = tpalloc( X_OCTET, nullptr, size);
               unittest::random::range( range::make( buffer, size));
               return buffer;
            }

            namespace configuration
            {
               constexpr auto base = R"(
domain: 
   name: base

   groups: 
      - name: base
      - name: user
        dependencies: [ base]
      - name: gateway
        dependencies: [ user]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
)";

            } // configuration
            
         } // <unnamed>
      } // local
      
      TEST( test_domain_gateway_discovery, domain_chain_A_B_C__C_has_echo__B_has_forward__call_echo_from_A___expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto C = R"(
domain: 
   name: C

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7710
)";

         constexpr auto B = R"(
domain: 
   name: B

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7710
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7720
                  discovery:
                     forward: true
)";

         constexpr auto A = R"(
domain: 
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7720
)";

         // tool to make sure we're connected to the previous domain, since the 'timing' is not
         // deterministisc
         auto connected_to = []( auto remote)
         {
            auto state = local::state::gateway::until( [remote]( auto& state)
            {
               return predicate::boolean( algorithm::find_if( state.connections, [remote]( auto& connection)
               {
                  return connection.remote.name == remote;
               })); 
            });
         };

         local::Manager c{ { local::configuration::base, C}};
         local::Manager b{ { local::configuration::base, B}};
         connected_to( "C");
         local::Manager a{ { local::configuration::base, A}};
         connected_to( "B");

         {
            auto buffer = local::allocate( 128);
            auto len = tptypes( buffer, nullptr, nullptr);

            EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

            tpfree( buffer);
         }
      }


      TEST( test_domain_gateway_discovery, domain_chain_A_B_C__C_has_echo__B_has_NOT_forward__call_echo_from_A___expect_TPENOENT)
      {
         common::unittest::Trace trace;

         constexpr auto C = R"(
domain: 
   name: C

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7710
)";

         constexpr auto B = R"(
domain: 
   name: B

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7710
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7720
)";

         constexpr auto A = R"(
domain: 
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7720
)";

         // tool to make sure we're connected to the previous domain, since the 'timing' is not
         // deterministisc
         auto connected_to = []( auto remote)
         {
            auto state = local::state::gateway::until( [remote]( auto& state)
            {
               return predicate::boolean( algorithm::find_if( state.connections, [remote]( auto& connection)
               {
                  return connection.remote.name == remote;
               })); 
            });
         };

         local::Manager c{ { local::configuration::base, C}};
         local::Manager b{ { local::configuration::base, B}};
         connected_to( "C");
         local::Manager a{ { local::configuration::base, A}};
         connected_to( "B");

         {
            auto buffer = local::allocate( 128);
            auto len = tptypes( buffer, nullptr, nullptr);

            EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, 0) == -1);
            EXPECT_TRUE( tperrno == TPENOENT) << "tperrno: " << tperrnostring( tperrno);

            tpfree( buffer);
         }
      }

   } // test::domain
} // casual
