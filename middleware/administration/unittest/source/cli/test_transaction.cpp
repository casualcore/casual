//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#define CASUAL_NO_XATMI_UNDEFINE

#include "common/transaction/context.h"

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

      TEST( cli_transaction, list_transactions)
      {
         common::unittest::Trace trace;

         auto a = local::cli::domain( R"(
system:
   resources:
      -  key: rm-mockup
         server: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/rm-proxy-casual-mockup
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm

domain: 
   name: A

   transaction:
      resources:
         -  key: rm-mockup
            name: example-resource-server
            instances: 1

   servers:
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server
         memberships: [ user]
)");
         EXPECT_TRUE( administration::unittest::cli::command::execute( R"(casual transaction --list-transactions --porcelain true)").string().empty());

         ASSERT_TRUE( tx_begin() == TX_OK);

         // create some branches
         local::cli::call( "casual/example/resource/branch/echo");
         local::cli::call( "casual/example/resource/branch/echo");
         local::cli::call( "casual/example/resource/branch/echo");

         // gtrid
         {
            const auto output = administration::unittest::cli::command::execute( R"(casual transaction --list-transactions --porcelain true | awk -F'|' '{printf $1}')").string();
            auto& trid = common::transaction::context().current().trid;
            EXPECT_EQ( output, common::string::compose( common::transaction::id::range::global( trid))) << "output: " << output << "expected: " << trid;
         }

         // branches
         {
            const auto output = administration::unittest::cli::command::execute( R"(casual transaction --list-transactions --porcelain true | awk -F'|' '{printf $2}')").string();
            constexpr auto expected = R"(3)";
            EXPECT_EQ( output, expected) << "output: " << output << "expected: " << expected;
         }

         // owner
         {
            const auto output = administration::unittest::cli::command::execute( R"(casual transaction --list-transactions --porcelain true | awk -F'|' '{printf $3}')").string();
            constexpr auto expected = R"(-)";
            EXPECT_EQ( output, expected) << "output: " << output << "expected: " << expected;
         }

         // stage
         {
            const auto output = administration::unittest::cli::command::execute( R"(casual transaction --list-transactions --porcelain true | awk -F'|' '{printf $4}')").string();
            constexpr auto expected = R"(involved)";
            EXPECT_EQ( output, expected) << "output: " << output << "expected: " << expected;
         }

         // resources
         {
            const auto output = administration::unittest::cli::command::execute( R"(casual transaction --list-transactions --porcelain true | awk -F'|' '{printf $5}')").string();
            constexpr auto expected = R"([L-1])";
            EXPECT_EQ( output, expected) << "output: " << output << "expected: " << expected;
         }

         ASSERT_TRUE( tx_commit() == TX_OK);
      }

      TEST( cli_transaction, legend)
      {
         auto a = local::cli::domain();

         const auto output = administration::unittest::cli::command::execute( R"(casual transaction --legend list-resources)").consume();

         using namespace std::literals;

         // check some legend specific strings
         EXPECT_TRUE( algorithm::search( output, "min:"sv));
         EXPECT_TRUE( algorithm::search( output, "openinfo:"sv));
         EXPECT_TRUE( algorithm::search( output, "P:"sv)) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( algorithm::search( output, "PAT:"sv));
      }

      TEST( cli_transaction, pending_resource_proxies)
      {
         auto a = local::cli::domain( R"(
system:
   resources:
      -  key: rm-mockup
         server: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/rm-proxy-casual-mockup
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm
domain: 
   name: A
   transaction:
      resources:
         -  key: rm-mockup
            openinfo: --sleep-commit 20ms
            name: example-resource-server
            instances: 1

   servers:
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/echo]
         instances: 4

)");

         // do 2 asynchronous calls to casual/example/resource/nested/calls/A that will start a transaction (auto)
         // call casual/example/resource/echo -> distributed transaction. TM will do the 2pc (one involved resource -> one-phase-commit-optimisation),
         // and we should get pending request to resource-proxy since we only got one instance.
         auto correlations = common::array::make( 
            common::unittest::service::send( "casual/example/resource/nested/calls/A", common::unittest::random::binary( 512)),
            common::unittest::service::send( "casual/example/resource/nested/calls/A", common::unittest::random::binary( 512)));

         // collect and discard replies
         algorithm::for_each( correlations, []( auto& correlation)
         {
            auto reply = communication::ipc::receive< common::message::service::call::Reply>( correlation);
            EXPECT_TRUE( reply.buffer.data.size() == 512);
         });

         // order of columns
         // ----------------
         // name:
         // id: 
         // key:
         // openinfo:
         // closeinfo:
         // invoked:
         // min:
         // max:
         // avg:
         // #:
         // P:
         // PAT:

         auto output = string::split( administration::unittest::cli::command::execute( R"(casual --porcelain true transaction --list-resources)").consume(), '|');
         ASSERT_TRUE( output.size() == 12);
         EXPECT_TRUE( output[ 0] == "example-resource-server");
         EXPECT_TRUE( output[ 1] == "L-1");
         EXPECT_TRUE( output[ 2] == "rm-mockup");
         //EXPECT_TRUE( output[ 3] == ""); // openinfo
         //EXPECT_TRUE( output[ 4] == ""); // closeinfo
         EXPECT_TRUE( string::from< long>( output[ 5]) == 2); // invoked
         EXPECT_TRUE( string::from< double>( output[ 6]) >= 0.02); // min
         EXPECT_TRUE( string::from< double>( output[ 7]) >= 0.02); // max
         EXPECT_TRUE( string::from< double>( output[ 8]) >= 0.02); // avg
         EXPECT_TRUE( string::from< long>( output[ 9]) == 1); // #
         EXPECT_TRUE( string::from< long>( output[ 10]) == 1); // P
         EXPECT_TRUE( string::from< double>( output[ 11]) > 0.01); // PAT should be close to 0.02 but to be safe we expect at least half.
      }

   } // administration
} // casual
