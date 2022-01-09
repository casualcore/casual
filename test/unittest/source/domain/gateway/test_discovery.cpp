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

#include "service/unittest/utility.h"
#include "gateway/unittest/utility.h"

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
            template< typename... C>
            auto manager( C&&... configurations)
            {
               return casual::domain::manager::unittest::process( std::forward< C>( configurations)...);
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


         auto c = local::manager( local::configuration::base, C);
         auto b = local::manager( local::configuration::base, B);
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());
         auto a = local::manager( local::configuration::base, A);
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

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


         auto c = local::manager( local::configuration::base, C);
         auto b = local::manager( local::configuration::base, B);
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());
         auto a = local::manager( local::configuration::base, A);
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         {
            auto buffer = local::allocate( 128);
            auto len = tptypes( buffer, nullptr, nullptr);

            EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, 0) == -1);
            EXPECT_TRUE( tperrno == TPENOENT) << "tperrno: " << tperrnostring( tperrno);

            tpfree( buffer);
         }
      }

      TEST( test_domain_gateway_discovery, A_to_B_C__C_is_down__expect_B___boot_C__expect_discovery_to_C__alternate_between_B_and_C)
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
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( { "B"}));

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
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( { "B", "C"}));

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

   } // test::domain
} // casual
