//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/unittest/manager.h"

#include "administration/unittest/cli/command.h"

#include "gateway/unittest/utility.h"

#include "casual/tx.h"
#include "casual/xatmi.h"

namespace casual
{
   using namespace common;

   namespace administration
   {
      namespace local
      {
         namespace
         {
            namespace cli
            {
               constexpr auto base = R"(
domain:
   groups:
      -  name: base
      -  name: user
         dependencies: [ base]
      -  name: gateway
         dependencies: [ user]
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
         memberships: [ gateway]

)";

               template< typename... C>
               auto domain( C&&... configurations)
               {
                  return casual::domain::unittest::manager( base, std::forward< C>( configurations)...);
               }

               auto call( std::string_view service)
               {
                  auto buffer = tpalloc( X_OCTET, nullptr, 128);
                  common::unittest::random::range( range::make( buffer, 128));
                  auto len = tptypes( buffer, nullptr, nullptr);

                  tpcall( service.data(), buffer, 128, &buffer, &len, 0);
                  EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);

                  return memory::guard( buffer, &tpfree);
               };

            } // cli
         } // <unnamed>
      } // local

      TEST( cli_transaction, list_external_resources__expect_outbound_gateways_to_be_listed)
      {
         common::unittest::Trace trace;

         auto b = local::cli::domain( R"(
domain: 
   name: B
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7001
)");

         auto a = local::cli::domain( R"(
domain: 
   name: A

   gateway:
      outbound:
         groups:
            -  alias: outbound_B
               connections:
                  -  address: 127.0.0.1:7001
)");
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         EXPECT_TRUE( administration::unittest::cli::command::execute( R"(casual transaction --list-external-resources --porcelain true)").string().empty());

         // Call B in transaction to 'discover' outbound as external resource
         ASSERT_TRUE( tx_begin() == TX_OK);
         local::cli::call( "casual/example/domain/echo/B");
         ASSERT_TRUE( tx_commit() == TX_OK);

         std::string output = administration::unittest::cli::command::execute( R"(casual transaction --list-external-resources --porcelain true | cut -d '|' -f 2)").string();

         constexpr auto expected = R"(outbound_B
)";
         EXPECT_TRUE( output == expected) << "output:  " << output << "expected: " << expected;
      }

   } // administration
} // casual
