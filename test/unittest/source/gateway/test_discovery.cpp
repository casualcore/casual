//! 
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "domain/unittest/manager.h"
#include "domain/discovery/api.h"

#include "common/communication/instance.h"

#include "queue/api/queue.h"
#include "queue/code.h"

#include "service/unittest/utility.h"
#include "gateway/unittest/utility.h"

#include "casual/xatmi.h"

namespace casual
{
   using namespace common;

   namespace test
   {

      namespace local
      {
         namespace
         {
            template< typename... C>
            auto manager( C&&... configurations)
            {
               return casual::domain::unittest::manager( std::forward< C>( configurations)...);
            }

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
      - name: queue
        dependencies: [ base]
      - name: gateway
        dependencies: [ queue]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager"
        memberships: [ queue]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
)";

            } // configuration
         
         } // <unnamed>
      } // local
      
      TEST( test_gateway_discovery, domain_chain_A_B_C__C_has_echo__B_has_forward__call_echo_from_A___expect_discovery)
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


         auto c = local::manager( local::configuration::base, C);
         auto b = local::manager( local::configuration::base, B);
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "C"));

         auto a = local::manager( local::configuration::base, A);
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "B"));

         {
            auto buffer = local::allocate( 128);
            auto len = tptypes( buffer, nullptr, nullptr);

            EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

            tpfree( buffer);
         }
      }


      TEST( test_gateway_discovery, domain_chain_A_B_C__C_has_echo__B_has_NOT_forward__call_echo_from_A___expect_TPENOENT)
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


         auto c = local::manager( local::configuration::base, C);
         auto b = local::manager( local::configuration::base, B);
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "C"));
         auto a = local::manager( local::configuration::base, A);
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "B"));

         {
            auto buffer = local::allocate( 128);
            auto len = tptypes( buffer, nullptr, nullptr);

            EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, 0) == -1);
            EXPECT_TRUE( tperrno == TPENOENT) << "tperrno: " << tperrnostring( tperrno);

            tpfree( buffer);
         }
      }

      TEST( test_gateway_discovery, domain_chain_A_B_C__C_has_echo_with_route_x__B_has_forward__call_x_from_A___expect_able_to_call_x)
      {
         common::unittest::Trace trace;

          auto c = local::manager( local::configuration::base, R"(
domain: 
   name: C
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   services:
      -  name: casual/example/echo
         routes: [ echo]
   gateway:
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7710
)");

         auto b = local::manager( local::configuration::base, R"(
domain: 
   name: B
   services:
      -  name: echo
         routes: [ x]
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
)");

         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "C"));

         auto a = local::manager( local::configuration::base, R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7720
)");
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "B"));


         {
            auto buffer = local::allocate( 128);
            auto len = tptypes( buffer, nullptr, nullptr);

            EXPECT_TRUE( tpcall( "x", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

            tpfree( buffer);
         }
      }

      TEST( test_gateway_discovery, A_to_B_C__C_is_down__expect_B___boot_C__expect_discovery_to_C__alternate_between_B_and_C)
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
               -  address: 127.0.0.1:7001
)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7002
)";

         constexpr auto A = R"(
domain: 
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7001
               -  address: 127.0.0.1:7002
)";


         
         auto b = local::manager( local::configuration::base, B);
         auto a = local::manager( local::configuration::base, A);
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "B"));

         auto call_domain_name = []()
         {
            auto buffer = local::allocate( 128);
            auto len = tptypes( buffer, nullptr, nullptr);
            std::string result;

            if( tpcall( "casual/example/domain/name", buffer, 128, &buffer, &len, 0) != -1)
               result = buffer;
            
            tpfree( buffer);
            return result;
         };

         algorithm::for_n< 10>( [&call_domain_name]()
         {
            EXPECT_TRUE( call_domain_name() == "B");
         });

         auto c = local::manager( local::configuration::base, C);

         // make sure we're _in_ domain A
         a.activate();
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "C"));

         {
            std::map< std::string, int> domain_count;

            algorithm::for_n< 20>( [&]()
            {
               domain_count[ call_domain_name()]++;
            });

            ASSERT_TRUE( domain_count.size() == 2) << CASUAL_NAMED_VALUE( domain_count);
            EXPECT_TRUE( domain_count.at( "B") > 0);
            EXPECT_TRUE( domain_count.at( "C") > 0);
         }
      }


      TEST( test_domain_gateway_discovery, domain_A_to_B_C__C_echo_not_discoverable____expect_TPENOENT_for_echo_C)
      {
         common::unittest::Trace trace;

         constexpr auto C = R"(
domain: 
   name: C      
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
   services:
      -  name: casual/example/domain/echo/C
         visibility: undiscoverable
   gateway:
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7010
)";

         constexpr auto B = R"(
domain: 
   name: B
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7011
)";


         constexpr auto A = R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7010
               -  address: 127.0.0.1:7011
)";


         auto c = local::manager( local::configuration::base, C);
         auto b = local::manager( local::configuration::base, B);
         auto a = local::manager( local::configuration::base, A);
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         // expect to find stuff in B
         {
            auto buffer = local::allocate( 128);
            auto len = tptypes( buffer, nullptr, nullptr);

            EXPECT_TRUE( tpcall( "casual/example/domain/echo/B", buffer, 128, &buffer, &len, 0) != -1) << "tperrno: " << tperrnostring( tperrno);
            tpfree( buffer);
         }

         // expect NOT to find stuff in C
         {
            auto expect_tpnoent = []( auto&& service)
            {
               auto buffer = local::allocate( 128);
               auto len = tptypes( buffer, nullptr, nullptr);

               EXPECT_TRUE( tpcall( "casual/example/domain/echo/C", buffer, 128, &buffer, &len, 0) == -1);
               EXPECT_TRUE( tperrno == TPENOENT) << "service: " << service << ", tperrno: " << tperrnostring( tperrno); 
               tpfree( buffer);
            };

            // the configured undiscoverable
            expect_tpnoent( "casual/example/domain/echo/C");

            // the built undiscoverable
            expect_tpnoent( "casual/example/undiscoverable/echo");
         }
      }

      TEST( test_gateway_discovery, A_to_B__topology_registration__B_to_C__expect__topology_update)
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
               -  address: 127.0.0.1:7010
)";

         auto b = local::manager( local::configuration::base, R"(
domain: 
   name: B
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7010
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7020
                  discovery:
                     forward: true
)");

         auto a = local::manager( local::configuration::base, R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7020
)");

         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( "B"));

         // make sure we get the topology update
         casual::domain::discovery::provider::registration( casual::domain::discovery::provider::Ability::topology);

         // boot C - topology update should propagate to A
         auto c = local::manager( local::configuration::base, C);

         a.activate();

         {
            // we need to _fetch until_ since we could get topology-implicit-update from the A -> B connect established first.
            constexpr auto fetch_topology_until = common::unittest::fetch::until( &common::communication::ipc::receive< casual::domain::message::discovery::topology::implicit::Update>);

            auto update = fetch_topology_until( []( auto& update)
            {
               return update.domains.size() == 2;
            });

            EXPECT_TRUE( algorithm::find( update.domains, "B")) << CASUAL_NAMED_VALUE( update.domains);
            EXPECT_TRUE( algorithm::find( update.domains, "A")) << CASUAL_NAMED_VALUE( update.domains);
         }

      }

      TEST( test_gateway_discovery, A_queue_forward_to_C__via_B__boot_A_B__then_boot_C__expect_forward_from_A)
      {
         common::unittest::Trace trace;

         constexpr auto C = R"(
domain: 
   name: C
   queue:
      groups:
         -  alias: C
            queuebase: ":memory:"
            queues:
               -  name: c1
   gateway:
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7010
)";

         auto b = local::manager( local::configuration::base, R"(
domain: 
   name: B
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7010
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7020
                  discovery:
                     forward: true
)");

         auto a = local::manager( local::configuration::base, R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7020

   queue:
      groups:
         -  alias: A
            queuebase: ":memory:"
            queues:
               -  name: a1
      forward:
         groups:
            -  alias: F-A
               queues:
                  -  source: a1
                     target:
                        queue: c1
                     instances: 1


)");
      
         queue::enqueue( "a1", { { ".binary", common::unittest::random::binary( 512)}});

         // boot C - topology update should propagate to A, and the queue forward kicks in
         auto c = local::manager( local::configuration::base, C);

         auto message = queue::blocking::available::dequeue( "c1");
         EXPECT_TRUE( message.payload.data.size() == 512);

      }

   } // test
} // casual
