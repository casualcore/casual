//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/code/tx.h"

namespace casual
{
   namespace common
   {

      TEST( common_code, tx_accumulate)
      {
         unittest::Trace trace;

         auto test_code = []( auto lhs, auto rhs, auto result)
         {
            EXPECT_TRUE( lhs + rhs == result);
            EXPECT_TRUE( rhs + lhs == result);
            return lhs + rhs == result && rhs + lhs == result;
         };

         // no begin combination
         EXPECT_TRUE( test_code( code::tx::not_supported, code::tx::argument, code::tx::not_supported));
         EXPECT_TRUE( test_code( code::tx::no_begin , code::tx::rollback, code::tx::no_begin_rollback));
         EXPECT_TRUE( test_code( code::tx::no_begin , code::tx::committed, code::tx::no_begin_committed));
         EXPECT_TRUE( test_code( code::tx::no_begin , code::tx::mixed, code::tx::no_begin_mixed));
         EXPECT_TRUE( test_code( code::tx::no_begin , code::tx::hazard, code::tx::no_begin_hazard));


         EXPECT_TRUE( test_code( code::tx::ok, code::tx::no_begin, code::tx::no_begin));

         EXPECT_TRUE( test_code( code::tx::ok, code::tx::rollback, code::tx::rollback));

         

      }
      
   } // common
} // casual