//!
//! test_transactionlog.cpp
//!
//! Created on: Nov 3, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "transaction/manager/log.h"

#include "common/file.h"

namespace casual
{
   namespace transaction
   {
      namespace local
      {
         common::message::transaction::begin::Request beginReqest()
         {
            common::message::transaction::begin::Request result;

            result.xid = common::transaction::ID::create();
            result.id.queue_id = 100;
            result.start = common::clock_type::now();


            return result;
         }

         common::file::ScopedPath transactionLogPath()
         {
            return common::file::ScopedPath{ "unittest_transaction_log.db"};
         }

      } // local

      TEST( casual_transaction_log, one_begin__expect_store)
      {
         auto path = local::transactionLogPath();
         Log log( path);

         auto begin = local::beginReqest();

         log.begin( begin);


         auto row = log.select( begin.xid);

         ASSERT_TRUE( row.size() == 1);
         EXPECT_TRUE( row.front().xid == begin.xid) << "global: " << row.front().xid.stringGlobal() << std::endl;

      }

      TEST( casual_transaction_log, two_begin__expect_store)
      {
         auto path = local::transactionLogPath();
         Log log( path);

         auto first = local::beginReqest();
         log.begin( first);

         auto second = local::beginReqest();
         log.begin( second);


         auto rows = log.select();

         ASSERT_TRUE( rows.size() == 2);
         EXPECT_TRUE( rows.at( 0).xid == first.xid);
         EXPECT_TRUE( rows.at( 1).xid == second.xid);

         EXPECT_TRUE( rows.at( 0).xid != rows.at( 1).xid);

      }

      TEST( casual_transaction_log, first_begin_then_prepare__expect_state_change)
      {
         auto path = local::transactionLogPath();
         Log log( path);

         auto begin = local::beginReqest();
         log.begin( begin);

         log.prepareCommit( begin.xid);


         auto rows = log.select( begin.xid);

         ASSERT_TRUE( rows.size() == 1);
         EXPECT_TRUE( rows.at( 0).xid == begin.xid);
         EXPECT_TRUE( rows.at( 0).state == Log::State::cPreparedCommit);
      }


   } // transaction

} // casual
