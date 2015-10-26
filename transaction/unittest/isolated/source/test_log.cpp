//!
//! test_transactionlog.cpp
//!
//! Created on: Nov 3, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "transaction/manager/log.h"
#include "transaction/manager/state.h"

#include "common/file.h"

namespace casual
{
   namespace transaction
   {
      namespace local
      {
         namespace
         {

            Transaction create_transaction()
            {
               Transaction result;

               result.trid = common::transaction::ID::create( common::process::handle());
               result.started = common::platform::clock_type::now();
               result.deadline = result.started + std::chrono::seconds{ 10};

               return result;
            }

            std::string transactionLogPath()
            {
               return ":memory:";
            }
         }

      } // local


      TEST( casual_transaction_log, prepare)
      {
         Log log( local::transactionLogPath());

         auto trans = local::create_transaction();

         log.prepare( trans);

         auto rows = log.logged();

         ASSERT_TRUE( rows.size() == 1);
         EXPECT_TRUE( rows.at( 0).trid == trans.trid);
         EXPECT_TRUE( rows.at( 0).state == Log::State::cPrepared);
      }


   } // transaction
} // casual
