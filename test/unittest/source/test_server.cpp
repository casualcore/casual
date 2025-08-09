//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "service/call/context.h"

#include "transaction/context.h"

#include "server/context.h"

#include "test/unittest/xatmi/buffer.h"

#include "domain/unittest/manager.h"

#include "casual/xatmi.h"
#include "casual/tx.h"



namespace casual
{
   namespace test::server
   {
      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto base = R"(
domain: 
   groups: 
      -  name: base
         dependencies: [ base]
      -  name: user
         dependencies: [ base]
      -  name: end
         dependencies: [ user]
   
   servers:
      -  path: "${CMAKE_BINARY_DIR}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CMAKE_BINARY_DIR}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
   
)";
   
            } // configuration

            template< typename... Cs>
            auto domain( Cs&&... configurations)
            {
               return casual::domain::unittest::manager( local::configuration::base, std::forward< Cs>( configurations)...);
            }
            
         } // <unnamed>
      } // local


      TEST( test_server, pending_transaction_calls__policy_finalize__expect_fetch_all_in_flight)
      {
         common::unittest::Trace trace;

         auto a = local::domain( R"(
domain: 
   name: A
   transaction:
      log: ":memory:"
   servers:         
      -  path: "${CMAKE_BINARY_DIR}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         instances: 4
)");  
         
         auto buffer = test::unittest::xatmi::buffer::x_octet{ 128L};
         
         // start a transaction 'manually'
         ASSERT_TRUE( ::tx_begin() == TX_OK ) << "tx_begin: " << tperrnostring( tperrno);
         
         common::algorithm::for_n< 2>( [ &]()
         {
            // in transaction
            EXPECT_TRUE( ::tpacall( "casual/example/echo", buffer.data, buffer.size, 0) != -1) << "tpacall: " << tperrnostring( tperrno);
            // outside transaction
            EXPECT_TRUE( ::tpacall( "casual/example/echo", buffer.data, buffer.size, TPNOTRAN) != -1) << "tpacall: " << tperrnostring( tperrno);
         });

         // should be 2 associated calls to the transaction
         EXPECT_TRUE( casual::transaction::context().associated().size() == 2);


         // emulate a rollback finalize
         casual::server::detail::finalize_transaction( false);

         // we should have cleared the in-flight calls
         EXPECT_TRUE( casual::service::call::context().empty());

         // we should have cleared the transaction context
         EXPECT_TRUE( casual::transaction::context().empty());

      }

      
   } // test::server
} // casual
