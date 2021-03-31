//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#define CASUAL_NO_XATMI_UNDEFINE

#include "domain/manager/unittest/process.h"

#include "common/communication/instance.h"
#include "common/transaction/context.h"
#include "common/unittest/rm.h"

#include "tx.h"

namespace casual
{
   using namespace common;

   namespace test
   {
      namespace domain
      {
         namespace local
         {
            namespace
            {
               auto domain( std::string_view configuration)
               {
                  return casual::domain::manager::unittest::Process{{
R"(
domain:
   name: test-default-domain

   groups: 
      - name: base
      - name: transaction
        dependencies: [ base]
      - name: queue
        dependencies: [ transaction]
      - name: example
        dependencies: [ queue]

   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ transaction]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
        memberships: [ queue]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-error-server
        memberships: [ example]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
        memberships: [ example]
)", configuration}};
            
               }


               struct Clear 
               {
                  Clear() { operator()();} 
                  ~Clear() { operator()();} 

                  void operator() () const
                  {
                     transaction::context().clear();
                     unittest::rm::clear();
                  }
               };

               auto id()
               {
                  auto ids = transaction::context().resources();
                  return ids.empty() ? strong::resource::id{} : ids.front();
               }

               constexpr auto configuration = R"(
domain: 
   name: dynamic-rm

   transaction:
      resources:
         - key: rm-mockup
           name: rm-mockup-dynamic
           openinfo:

)";

               transaction::resource::Link link()
               {
                  return { "rm-mockup", "rm-mockup-dynamic", &casual_mockup_xa_switch_dynamic};
               }
            } // <unnamed>
         } // local

         TEST( test_domain_transaction, empty_configuration)
         {
            unittest::Trace trace;
            local::Clear clear;

            EXPECT_TRUE( ! transaction::context().current());
         }

         TEST( test_domain_transaction, dynamic_resource_configure)
         {
            unittest::Trace trace;
            local::Clear clear;

            auto domain = local::domain( local::configuration);
            transaction::context().configure( { local::link()});

            EXPECT_TRUE( local::id() == strong::resource::id{ 1}) << "local::id(): " << local::id(); 
         }

         TEST( test_domain_transaction, dynamic_resource_not_involved__transaction_commit__expect_no_xa_end_invokation)
         {
            unittest::Trace trace;
            local::Clear clear;

            auto domain = local::domain( local::configuration);
            transaction::context().configure( { local::link()});

            EXPECT_TRUE( tx_begin() == TX_OK);
            // no rm involvement
            EXPECT_TRUE( tx_commit() == TX_OK);

            auto state = unittest::rm::state( local::id());
            EXPECT_TRUE( state.errors.empty()) << state.errors;
            // only open has been called
            ASSERT_TRUE( state.invocations.size() == 1) << state.invocations;
            ASSERT_TRUE( state.invocations.at( 0) == unittest::rm::State::Invoke::xa_open_entry) << state.invocations;
         }

         TEST( test_domain_transaction, dynamic_resource_involved__transaction_commit__expect_xa_end_invokation)
         {
            unittest::Trace trace;
            local::Clear clear;

            auto domain = local::domain( local::configuration);
            transaction::context().configure( { local::link()});

            auto id = local::id();

            EXPECT_TRUE( tx_begin() == TX_OK);
            unittest::rm::registration( id);
            EXPECT_TRUE( tx_commit() == TX_OK);

            auto state = unittest::rm::state( id);
            EXPECT_TRUE( state.errors.empty()) << state.errors;

            using Invoke = unittest::rm::State::Invoke;
            
            // configure
            //   -> xa_open_entry
            // tx_begin 
            //   -> no xa_start since resource is dynamic
            // resource registration 
            //    -> rm gets involved
            // tx_commit (local transaction)
            //   -> xa_end_entry
            //   -> xa_commit_entry
            // 
            const auto expected = std::vector< Invoke>{ Invoke::xa_open_entry, Invoke::xa_end_entry, Invoke::xa_commit_entry};
            EXPECT_TRUE( state.invocations == expected) << state.invocations;
         }
         
      } // domain
   } // test
} // casual