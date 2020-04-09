//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "transaction/manager/state.h"

namespace casual
{
   namespace transaction
   {
      namespace local
      {
         namespace
         {
            auto rm_1 = common::strong::resource::id{ 1};
            auto rm_2 = common::strong::resource::id{ 2};
         } // <unnamed>
      } // local

      TEST( transaction_manager_state, instantiation)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW(({
            manager::State state{ manager::Settings{ sql::database::memory::file}, {}, {}};
         }));
      }

      TEST( transaction_manager_state_transaction, instantiation)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            manager::Transaction transaction{ common::transaction::id::create()};
         });
      }

      TEST( transaction_manager_state_transaction, stage__resources__1_involved__1__involved___gives__involved)
      {
         common::unittest::Trace trace;

         manager::Transaction transaction{ common::transaction::id::create()};

         auto& branch = transaction.branches.back();

         {
            branch.involve( local::rm_1);
            branch.involve( local::rm_2);

            ASSERT_TRUE( branch.resources.size() == 2);
         }

         EXPECT_TRUE( transaction.stage() == manager::Transaction::Resource::Stage::involved) << CASUAL_NAMED_VALUE( transaction);
      }

      TEST( transaction_manager_state_transaction, stage__1_branch__rm__1_involved__1__prepare_requested___gives__involved)
      {
         common::unittest::Trace trace;

         manager::Transaction transaction{ common::transaction::id::create()};

         auto& branch = transaction.branches.back();

         {
            branch.involve( local::rm_1);
            branch.involve( local::rm_2);

            ASSERT_TRUE( branch.resources.size() == 2);
            branch.resources.back().stage = manager::Transaction::Resource::Stage::prepare_requested;
         }

         EXPECT_TRUE( transaction.stage() == manager::Transaction::Resource::Stage::involved) << CASUAL_NAMED_VALUE( transaction);
      }

      TEST( transaction_manager_state_transaction, stage__branch1__rm1_involved__branch2__rm2_prepare_requested___gives__involved)
      {
         common::unittest::Trace trace;

         auto trid = common::transaction::id::create();
         manager::Transaction transaction{ trid};
         transaction.branches.emplace_back( common::transaction::id::branch( trid));

         {
            auto& branch = transaction.branches.at( 0);
            branch.involve( local::rm_1);
         }
         

         {
            auto& branch = transaction.branches.at( 1);
            branch.involve( local::rm_2);

            ASSERT_TRUE( branch.resources.size() == 1);
            branch.resources.back().stage = manager::Transaction::Resource::Stage::prepare_requested;
         }

         EXPECT_TRUE( transaction.stage() == manager::Transaction::Resource::Stage::involved) << CASUAL_NAMED_VALUE( transaction);
      }

      TEST( transaction_manager_state_transaction, stage__branch1__rm1_involved_rm2_prepare_requested__branch2__rm2_prepare_requested___gives__involved)
      {
         common::unittest::Trace trace;

         auto trid = common::transaction::id::create();
         manager::Transaction transaction{ trid};
         transaction.branches.emplace_back( common::transaction::id::branch( trid));

         {
            auto& branch = transaction.branches.at( 0);
            branch.involve( local::rm_1);
            branch.involve( local::rm_2);

            ASSERT_TRUE( branch.resources.size() == 2);
            branch.resources.back().stage = manager::Transaction::Resource::Stage::prepare_requested;
         }
         
         {
            auto& branch = transaction.branches.at( 1);
            branch.involve( local::rm_2);

            ASSERT_TRUE( branch.resources.size() == 1);
            branch.resources.back().stage = manager::Transaction::Resource::Stage::prepare_requested;
         }


         EXPECT_TRUE( transaction.resource_count() == 3);
         EXPECT_TRUE( transaction.stage() == manager::Transaction::Resource::Stage::involved) << CASUAL_NAMED_VALUE( transaction);
      }
   } // transaction
} // casual