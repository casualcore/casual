//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"


#include "transaction/manager/log.h"
#include "transaction/manager/state.h"

namespace casual
{
   namespace transaction
   {
      namespace local
      {
         namespace
         {

            manager::Transaction create_transaction()
            {
               manager::Transaction result{ common::transaction::id::create( common::process::handle())};

               result.started = platform::time::clock::type::now();
               result.deadline = result.started + std::chrono::seconds{ 10};

               return result;
            }

            constexpr auto log_path = ":memory:";
         }
      } // local


      TEST( transaction_manager_log, prepare)
      {
         common::unittest::Trace trace;

         manager::Log log( local::log_path);

         auto trans = local::create_transaction();

         log.prepare( trans);

         auto rows = log.logged();

         ASSERT_TRUE( rows.size() == 1);
         EXPECT_TRUE( rows.at( 0).trid == trans.branches.at( 0).trid);
         EXPECT_TRUE( rows.at( 0).state == manager::Log::State::prepared);
      }


   } // transaction
} // casual
