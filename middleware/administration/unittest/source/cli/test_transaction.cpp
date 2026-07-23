//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#define CASUAL_NO_XATMI_UNDEFINE


#include "domain/unittest/manager.h"
#include "domain/unittest/discover.h"

#include "administration/unittest/cli/command.h"

#include "gateway/unittest/utility.h"

#include "transaction/context.h"

#include "service/unittest/utility.h"

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
      -  name: queue
         dependencies: [ base]
      -  name: user
         dependencies: [ queue]
      -  name: gateway
         dependencies: [ user]
   servers:
      -  path: "${CMAKE_BINARY_DIR}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CMAKE_BINARY_DIR}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "${CMAKE_BINARY_DIR}/middleware/queue/bin/casual-queue-manager"
         memberships: [ base]
      -  path: "${CMAKE_BINARY_DIR}/middleware/gateway/bin/casual-gateway-manager"
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


               auto execute_get_lines( auto command)
               {
                  auto capture = administration::unittest::cli::command::execute( command);
                  EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture);

                  return string::split( capture.standard.out, '\n');
               };

            } // cli
         } // <unnamed>
      } // local

      TEST( cli_transaction, list_resource_instances_external__expect_outbound_gateways_to_be_listed)
      {
         common::unittest::Trace trace;

         auto b = local::cli::domain( R"(
domain: 
   name: domain-B
   servers:
      -  path: "${CMAKE_BINARY_DIR}/middleware/example/server/bin/casual-example-server"
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
            -  alias: outbound-B
               connections:
                  -  address: 127.0.0.1:7001
                  -  address: 127.0.0.1:7001
                  -  address: 127.0.0.1:7001
)");
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         {

            //EXPECT_TRUE( false) << administration::unittest::cli::command::execute( R"(casual transaction --list-resource-instances --external)").standard.out;

/*
id   alias       pid    ipc                               description
---  ----------  -----  --------------------------------  -----------
E-1  outbound-B  51294  58355322c4654c22bccaa9707da71ff2  domain-B   
E-2  outbound-B  51294  efa9db8f60714107b713a5bd01edc260  domain-B   
E-3  outbound-B  51294  7a3ba66c51d14e6ab6387c522d7518d9  domain-B
*/


            auto lines = local::cli::execute_get_lines( R"(casual --header false --color false transaction --list-resource-instances --external)");
            
            auto e1 = string::adjacent::split( lines.at( 0), ' ');
            EXPECT_TRUE( e1.at( 0) == "E-1");
            EXPECT_TRUE( e1.at( 1) == "outbound-B");
            EXPECT_TRUE( ! e1.at( 2).empty());
            EXPECT_TRUE( ! e1.at( 3).empty());
            EXPECT_TRUE( e1.at( 4) == "domain-B");

            auto e2 = string::adjacent::split( lines.at( 1), ' ');
            EXPECT_TRUE( e2.at( 0) == "E-2");
            EXPECT_TRUE( e2.at( 1) == "outbound-B");
            EXPECT_TRUE( ! e2.at( 2).empty());
            EXPECT_TRUE( ! e2.at( 3).empty());
            EXPECT_TRUE( e2.at( 4) == "domain-B");

            auto e3 = string::adjacent::split( lines.at( 2), ' ');
            EXPECT_TRUE( e3.at( 0) == "E-3");
            EXPECT_TRUE( e3.at( 1) == "outbound-B");
            EXPECT_TRUE( ! e3.at( 2).empty());
            EXPECT_TRUE( ! e3.at( 3).empty());
            EXPECT_TRUE( e3.at( 4) == "domain-B");
         }
      }

      TEST( cli_transaction, list_transactions)
      {
         common::unittest::Trace trace;

         auto a = local::cli::domain( R"(
system:
   resources:
      -  key: rm-mockup
         server: ${CMAKE_BINARY_DIR}/middleware/transaction/bin/rm-proxy-casual-mockup
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
      -  path: ${CMAKE_BINARY_DIR}/middleware/example/server/bin/casual-example-resource-server
         memberships: [ user]
)");
         EXPECT_TRUE( administration::unittest::cli::command::execute( R"(casual transaction --list-transactions --porcelain true)").standard.out.empty());

         ASSERT_TRUE( tx_begin() == TX_OK);

         // create some branches
         local::cli::call( "casual/example/resource/branch/echo");
         local::cli::call( "casual/example/resource/branch/echo");
         local::cli::call( "casual/example/resource/branch/echo");


/*
global                            #branches  owner  stage     known                             deadline  resources
--------------------------------  ---------  -----  --------  --------------------------------  --------  ---------
d34f921bcf8f43c486284bcc15d66439  3              -  involved  2025-08-12T12:47:27.626377+02:00  -         [L-1]  
*/


         auto lines = local::cli::execute_get_lines( R"(casual --header false --color false transaction --list-transactions)");

         auto& current = casual::transaction::context().current().trid;

         auto first = string::adjacent::split( lines.at( 0), ' ');
         EXPECT_TRUE( first.at( 0) == common::string::compose( current.global()));
         EXPECT_TRUE( first.at( 1) == "3"); // branches
         EXPECT_TRUE( first.at( 2) == "-"); // owner
         EXPECT_TRUE( first.at( 3) == "involved"); // stage
         EXPECT_TRUE( ! first.at( 4).empty()); // known
         EXPECT_TRUE( first.at( 5) == "-"); // deadline
         EXPECT_TRUE( first.at( 6) == "[L-1]"); // resources

         ASSERT_TRUE( tx_commit() == TX_OK);
      }

      TEST( cli_transaction, extended_help)
      {
         auto a = local::cli::domain();

         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual transaction --help --list-resources)");

            using namespace std::literals;

            // check some legend specific strings
            EXPECT_TRUE( algorithm::search( capture.standard.out, "min:"sv));
            EXPECT_TRUE( algorithm::search( capture.standard.out, "openinfo:"sv));
            EXPECT_TRUE( algorithm::search( capture.standard.out, "P:"sv)) << CASUAL_NAMED_VALUE( capture);
            EXPECT_TRUE( algorithm::search( capture.standard.out, "PAT:"sv));
         }

         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual transaction --list-transactions --help)");
            using namespace std::literals;
            // check some legend specific strings
            EXPECT_TRUE( algorithm::search( capture.standard.out, "global:"sv));
            EXPECT_TRUE( algorithm::search( capture.standard.out, "#branches:"sv));
            EXPECT_TRUE( algorithm::search( capture.standard.out, "owner:"sv));
            EXPECT_TRUE( algorithm::search( capture.standard.out, "stage:"sv));
            EXPECT_TRUE( algorithm::search( capture.standard.out, "known:"sv));
            EXPECT_TRUE( algorithm::search( capture.standard.out, "deadline:"sv));
            EXPECT_TRUE( algorithm::search( capture.standard.out, "resources:"sv));
         }
      }

      TEST( cli_transaction, pending_resource_proxies)
      {
         auto a = local::cli::domain( R"(
system:
   resources:
      -  key: rm-mockup
         server: ${CMAKE_BINARY_DIR}/middleware/transaction/bin/rm-proxy-casual-mockup
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
      -  path: ${CMAKE_BINARY_DIR}/middleware/example/server/bin/casual-example-resource-server
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/echo]
         instances: 4

)");
         // wait until we have all 4 instances of example-resource-server up'n running
         casual::service::unittest::fetch::until( 
            casual::service::unittest::fetch::predicate::instances( "casual/example/resource/echo", 4));  

         // do 2 asynchronous calls to casual/example/resource/nested/calls/A that will start a transaction (auto)
         // call casual/example/resource/echo -> distributed transaction. TM will do the 2pc (one involved 
         // resource -> one-phase-commit-optimisation),
         // and we should get pending request to resource-proxy since we only got one instance.
         auto correlations = std::array{ 
            casual::service::unittest::send::request( "casual/example/resource/nested/calls/A", common::unittest::random::binary( 512)),
            casual::service::unittest::send::request( "casual/example/resource/nested/calls/A", common::unittest::random::binary( 512))};
         

         // collect and discard replies
         algorithm::for_each( correlations, []( auto& correlation)
         {
            auto reply = communication::ipc::receive< common::message::service::call::Reply>( correlation);
            EXPECT_TRUE( reply.buffer.data.size() == 512);
         });

         //EXPECT_TRUE( false) << administration::unittest::cli::command::execute( R"(casual transaction --list-resources)").standard.out;
/*

name                     id   key        openinfo             closeinfo  #B  invoked  min    max    avg    P  PAT    #
-----------------------  ---  ---------  -------------------  ---------  --  -------  -----  -----  -----  -  -----  -
example-resource-server  L-1  rm-mockup  --sleep-commit 20ms  -           0        2  0.024  0.025  0.024  1  0.025  1
*/

         auto lines = local::cli::execute_get_lines( R"(casual --color false --header false transaction --list-resources)");

         auto first = string::adjacent::split( lines.at( 0), ' ');

         EXPECT_TRUE( first.at( 0) == "example-resource-server");
         EXPECT_TRUE( first.at( 1) == "L-1");
         EXPECT_TRUE( first.at( 2) == "rm-mockup");
         EXPECT_TRUE( first.at( 3) == "--sleep-commit" && first.at( 4) == "20ms"); // openinfo, two parts since the string contains a space
         EXPECT_TRUE( first.at( 5) == "-") <<  first.at( 5);  // closeinfo
         EXPECT_TRUE( string::from< long>( first.at( 6)) == 0); // #B
         EXPECT_TRUE( string::from< long>( first.at( 7)) == 2); // invoked
         EXPECT_TRUE( string::from< double>( first.at( 8)) >= 0.02); // min
         EXPECT_TRUE( string::from< double>( first.at( 9)) >= 0.02); // max
         EXPECT_TRUE( string::from< double>( first.at( 10)) >= 0.02); // avg
         EXPECT_TRUE( string::from< long>( first.at( 11)) == 1) << CASUAL_NAMED_VALUE( first); // P
         // PAT should be close to 0.02 but on slow system it can be lower.
         EXPECT_TRUE( string::from< double>( first.at( 12)) > 0.009) << CASUAL_NAMED_VALUE( first); 
         EXPECT_TRUE( string::from< long>( first.at( 13)) == 1); // #, should be 1 since we only have one instance
      }


      TEST( cli_transaction, list_resource_instances)
      {
         auto b = local::cli::domain( R"(
domain: 
   name: domain-B
   gateway:
      inbound:
         groups:
            - connections:
               -  address: 127.0.0.1:7001

)");

         auto a = local::cli::domain( R"(
system:
   resources:
      -  key: rm-mockup
         server: ${CMAKE_BINARY_DIR}/middleware/transaction/bin/rm-proxy-casual-mockup
         xa_struct_name: casual_mockup_xa_switch_static
domain: 
   name: domain-A
   transaction:
      resources:
         -  key: rm-mockup
            name: example-resource-server
            instances: 2
   queue:
      groups:
         -  alias: QGA1
            queues:
               -  name: a1;
         -  alias: QGA2
            queues:
               -  name: a2
   gateway:
      outbound:
         groups:
            - connections:
               -  address: 127.0.0.1:7001
               -  address: 127.0.0.1:7001
               -  address: 127.0.0.1:7001

)");
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

        
         //EXPECT_TRUE( false) << administration::unittest::cli::command::execute( R"(casual transaction --list-resource-instances)").standard.out;
/*
id   alias                    state     pid    ipc                               description
---  -----------------------  --------  -----  --------------------------------  -----------
L-1  example-resource-server  idle      69754  d6a33f0ee67a4e2f8552dd4c5cb5fbc2  -          
L-1  example-resource-server  idle      69755  2b94d744b3644ea285d36b236b2ced72  -          
E-1  QGA2                     external  69753  ceadda0bf63a47a08e37cbb7c97c7fd0  queue-group
E-2  QGA1                     external  69752  888c2f41069848208a46f1c89b884372  queue-group
E-3  outbound                 external  69757  3d752a95f77049118500b4b8a3418f42  domain-B   
E-4  outbound                 external  69757  258de77b43654fbfaa687ddea66e78c7  domain-B   
E-5  outbound                 external  69757  81603fe1d1db4653813110841a5fe7be  domain-B
*/
         

         auto check_rows = []( auto rows)
         {
            EXPECT_TRUE( std::regex_match( rows.at( 0), std::regex{ "L-1[ ]+example-resource-server[ ]+idle.*-[ ]*"}));
            EXPECT_TRUE( std::regex_match( rows.at( 1), std::regex{ "L-1[ ]+example-resource-server[ ]+idle.*-[ ]*"}));
            EXPECT_TRUE( std::regex_match( rows.at( 2), std::regex{ "E-1[ ]+QGA[1,2][ ]+external.*queue-group[ ]*"}));
            EXPECT_TRUE( std::regex_match( rows.at( 3), std::regex{ "E-2[ ]+QGA[1,2][ ]+external.*queue-group[ ]*"}));
            EXPECT_TRUE( std::regex_match( rows.at( 4), std::regex{ "E-3[ ]+outbound[ ]+external.*domain-B[ ]*"}));
            EXPECT_TRUE( std::regex_match( rows.at( 5), std::regex{ "E-4[ ]+outbound[ ]+external.*domain-B[ ]*"}));
            EXPECT_TRUE( std::regex_match( rows.at( 6), std::regex{ "E-5[ ]+outbound[ ]+external.*domain-B[ ]*"}));
         };

         check_rows( local::cli::execute_get_lines( R"(casual --header false --color false transaction --list-resource-instances --all)"));

         // should be the same with default flag (all)
         check_rows( local::cli::execute_get_lines( R"(casual --header false --color false transaction --list-resource-instances)"));

      }

   } // administration
} // casual
