//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#define CASUAL_NO_XATMI_UNDEFINE

#include "domain/unittest/manager.h"

#include "common/communication/instance.h"
#include "common/transaction/context.h"
#include "common/environment/scoped.h"
#include "common/unittest/rm.h"

#include "test/unittest/xatmi/buffer.h"

#include "service/unittest/utility.h"

#include "transaction/unittest/utility.h"

#include "tx.h"
#include "casual/xatmi.h"

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
               namespace configuration
               {
                  constexpr auto basic = R"(
system:
   resources:
      -  key: rm-mockup
         server: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/rm-proxy-casual-mockup
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm

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
)";

           
               } // configuration

               template< typename... C>
               auto domain( C&&... configurations) 
               {
                  return casual::domain::unittest::manager( configuration::basic, std::forward< C>( configurations)...);
               }


               struct Clear 
               {
                  Clear() { operator()();} 
                  ~Clear() { operator()();} 

                  void operator() () const
                  {
                     common::transaction::context().clear();
                     common::unittest::rm::clear();
                  }
               };

               auto id()
               {
                  auto ids = common::transaction::context().resources();
                  return ids.empty() ? strong::resource::id{} : ids.front();
               }

               common::transaction::resource::Link link( std::string name)
               {
                  return { "rm-mockup", std::move( name), &casual_mockup_xa_switch_dynamic};
               }

               common::transaction::resource::Link link_static( std::string name)
               {
                  return { "rm-mockup", std::move( name), &casual_mockup_xa_switch_static};
               }
            } // <unnamed>
         } // local

         TEST( test_transaction, empty_configuration)
         {
            common::unittest::Trace trace;
            local::Clear clear;

            EXPECT_TRUE( ! common::transaction::context().current());
         }

         TEST( test_transaction, dynamic_resource_configure)
         {
            common::unittest::Trace trace;
            local::Clear clear;
   
            constexpr auto configuration = R"(
domain: 
   name: dynamic-rm

   transaction:
      resources:
         - key: rm-mockup
           name: rm1
           openinfo: rm1

)";

            auto domain = local::domain( configuration);
            common::transaction::context().configure( { local::link( "rm1")});

            EXPECT_TRUE( local::id() == strong::resource::id{ 1}) << "local::id(): " << local::id(); 
         }

         TEST( test_transaction, dynamic_resource_not_involved__transaction_commit__expect_no_xa_end_invokation)
         {
            common::unittest::Trace trace;
            local::Clear clear;

            constexpr auto configuration = R"(
domain: 
   name: dynamic-rm

   transaction:
      resources:
         - key: rm-mockup
           name: rm2
           openinfo:

)";

            auto domain = local::domain( configuration);
            common::transaction::context().configure( { local::link( "rm2")});

            EXPECT_TRUE( tx_begin() == TX_OK);
            // no rm involvement
            EXPECT_TRUE( tx_commit() == TX_OK);

            auto state = common::unittest::rm::state( local::id());
            EXPECT_TRUE( state.errors.empty()) << CASUAL_NAMED_VALUE( state.errors);
            // only open has been called
            ASSERT_TRUE( state.invocations.size() == 1) << CASUAL_NAMED_VALUE( state.invocations);
            ASSERT_TRUE( state.invocations.at( 0) == common::unittest::rm::State::Invoke::xa_open_entry) << CASUAL_NAMED_VALUE( state.invocations);
         }

         TEST( test_transaction, dynamic_resource_involved__transaction_commit__expect_xa_end_invokation)
         {
            common::unittest::Trace trace;
            local::Clear clear;

            constexpr auto configuration = R"(
domain: 
   name: dynamic-rm

   transaction:
      resources:
         - key: rm-mockup
           name: rm3
           openinfo:

)";

            auto domain = local::domain( configuration);
            common::transaction::context().configure( { local::link( "rm3")});

            auto id = local::id();
            EXPECT_TRUE( id);

            EXPECT_TRUE( tx_begin() == TX_OK);
            common::unittest::rm::registration( id);
            EXPECT_TRUE( tx_commit() == TX_OK);

            auto state = common::unittest::rm::state( id);
            EXPECT_TRUE( state.errors.empty()) << CASUAL_NAMED_VALUE( state.errors);

            using Invoke = common::unittest::rm::State::Invoke;
            
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
            EXPECT_TRUE( state.invocations == expected) << CASUAL_NAMED_VALUE( state.invocations);
         }

         TEST( test_transaction, suspend_resume_during_tpcall)
         {
            common::unittest::Trace trace;
            local::Clear clear;

            constexpr auto configuration = R"(
domain: 
   name: dynamic-rm

   transaction:
      resources:
         -  key: rm-mockup
            name: rm1
            instances: 1


)";

            auto domain = local::domain( configuration);
            common::transaction::context().configure( { local::link_static( "rm1")});

            auto rm = local::id();
            using Invoke = common::unittest::rm::State::Invoke;

            EXPECT_TRUE( rm);
            EXPECT_TRUE( tx_begin() == TX_OK);

            {
               auto state = common::unittest::rm::state( rm);
               EXPECT_TRUE( algorithm::count( state.invocations, Invoke::xa_end_entry) == 0) << CASUAL_NAMED_VALUE( state);
               EXPECT_TRUE( algorithm::count( state.invocations, Invoke::xa_start_entry) == 1) << CASUAL_NAMED_VALUE( state);

               auto buffer = unittest::xatmi::buffer::x_octet{};
               EXPECT_TRUE( ::tpcall( "casual/example/echo", buffer.data, buffer.size, &buffer.data, &buffer.size, 0) != -1);
            }

            auto state = common::unittest::rm::state( rm);
            EXPECT_TRUE( state.transactions.all.size() == 1);
            EXPECT_TRUE( algorithm::count( state.invocations, Invoke::xa_end_entry) == 1);
            EXPECT_TRUE( algorithm::count( state.invocations, Invoke::xa_start_entry) == 2);

            EXPECT_TRUE( tx_commit() == TX_OK);
         }

         TEST( test_transaction, two_resources__tpcall_to_our_self__expekt_involved_rm_to_TM)
         {
            common::unittest::Trace trace;
            local::Clear clear;

            constexpr auto configuration = R"(
domain: 
   name: dynamic-rm

   transaction:
      resources:
         -  key: rm-mockup
            name: rm1
            instances: 1
         -  key: rm-mockup
            name: rm2
            instances: 1


)";

            auto domain = local::domain( configuration);

            common::transaction::context().configure( { local::link_static( "rm1"), local::link_static( "rm2")});
            const auto resources = common::transaction::context().resources();


            EXPECT_TRUE( tx_begin() == TX_OK);

            {
               auto buffer = unittest::xatmi::buffer::x_octet{};
               EXPECT_TRUE( ::tpcall( "casual/example/echo", buffer.data, buffer.size, &buffer.data, &buffer.size, 0) != -1);
            }

            {               
               auto state = casual::transaction::unittest::state();
               auto& branch = state.transactions.at( 0).branches.at( 0);

               EXPECT_TRUE( algorithm::includes( branch.resources, resources));
            }

            EXPECT_TRUE( tx_commit() == TX_OK);
         }

      } // domain
   } // test
} // casual