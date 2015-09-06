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
         common::message::transaction::begin::Request begin_request()
         {
            common::message::transaction::begin::Request result;


            result.trid = common::transaction::ID::create();
            result.process.queue = 100;

            //
            // We've got a 1s delay on the timeout (to get "better" semantics) , so we subtract 1s from 'now'
            //
            result.start = common::platform::clock_type::now() - std::chrono::seconds{ 1};

            return result;
         }

         std::string transactionLogPath()
         {
            return ":memory:";
         }

      } // local

      TEST( casual_transaction_log, one_begin__expect_store)
      {
         auto path = local::transactionLogPath();
         Log log( path);

         auto begin = local::begin_request();

         log.begin( begin);


         auto row = log.select( begin.trid);

         ASSERT_TRUE( row.size() == 1);
         EXPECT_TRUE( row.front().trid == begin.trid) << "row.front().trid: " << row.front().trid << std::endl;

      }

      TEST( casual_transaction_log, two_begin__expect_store)
      {
         auto path = local::transactionLogPath();
         Log log( path);

         auto first = local::begin_request();
         log.begin( first);

         auto second = local::begin_request();
         log.begin( second);


         auto rows = log.select();

         ASSERT_TRUE( rows.size() == 2);
         EXPECT_TRUE( rows.at( 0).trid == first.trid);
         EXPECT_TRUE( rows.at( 1).trid == second.trid);

         EXPECT_TRUE( rows.at( 0).trid != rows.at( 1).trid);

      }

      TEST( casual_transaction_log, first_begin_then_prepare__expect_state_change)
      {
         auto path = local::transactionLogPath();
         Log log( path);

         auto begin = local::begin_request();
         log.begin( begin);

         log.prepare( begin.trid);


         auto rows = log.select( begin.trid);

         ASSERT_TRUE( rows.size() == 1);
         EXPECT_TRUE( rows.at( 0).trid == begin.trid);
         EXPECT_TRUE( rows.at( 0).state == Log::State::cPrepared);
      }


   } // transaction
} // casual
