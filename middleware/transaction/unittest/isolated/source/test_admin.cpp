//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"


#include "transaction/manager/admin/transactionvo.h"
#include "transaction/manager/admin/transform.h"

#include "transaction/manager/state.h"


#include "sf/log.h"

namespace casual
{

   namespace transaction
   {
      
      TEST( casual_transaction_admin, statistics_start_end)
      {
         common::unittest::Trace trace;

         state::Statistics statistics;

         auto now = common::platform::time::clock::type::now();

         statistics.start( now);
         statistics.end( now + 100us);

         EXPECT_TRUE( statistics.invoked == 1);
         EXPECT_TRUE( statistics.min == 100us);
         EXPECT_TRUE( statistics.max == 100us);
         EXPECT_TRUE( statistics.total == 100us);

      }

      TEST( casual_transaction_admin, transform_statistics)
      {
         common::unittest::Trace trace;

         state::Statistics statistics;

         auto now = common::platform::time::clock::type::now();

         statistics.start( now);
         statistics.end( now + 100us);

         auto vo = transform::Statistics{}( statistics);

         EXPECT_TRUE( vo.invoked == 1);
         EXPECT_TRUE( vo.min == 100us);
         EXPECT_TRUE( vo.max == 100us);
         EXPECT_TRUE( vo.total == 100us);

      }

      TEST( casual_transaction_admin, vo_statistics__addition_assignment__rhs_defualt)
      {
         common::unittest::Trace trace;

         state::Statistics statistics;

         auto now = common::platform::time::clock::type::now();

         statistics.start( now);
         statistics.end( now + 100us);

         auto vo = transform::Statistics{}( statistics);
         vo += vo::Statistics{};

         EXPECT_TRUE( vo.invoked == 1);
         EXPECT_TRUE( vo.min == 100us);
         EXPECT_TRUE( vo.max == 100us);
         EXPECT_TRUE( vo.total == 100us);

      }

      TEST( casual_transaction_admin, vo_statistics__addition_assignment__lhs_defualt)
      {
         common::unittest::Trace trace;

         state::Statistics statistics;

         auto now = common::platform::time::clock::type::now();

         statistics.start( now);
         statistics.end( now + 100us);

         vo::Statistics vo;
         vo += transform::Statistics{}( statistics);

         EXPECT_TRUE( vo.invoked == 1);
         EXPECT_TRUE( vo.min == 100us) << CASUAL_MAKE_NVP( vo);
         EXPECT_TRUE( vo.max == 100us);
         EXPECT_TRUE( vo.total == 100us);

      }

      TEST( casual_transaction_admin, vo_statistics__addition_assignment__rhs_defualt_2x)
      {
         common::unittest::Trace trace;

         vo::Statistics vo;

         vo.min = 100us;
         vo.max = 100us + 100us;
         vo.total = 100us * 3;
         vo.invoked = 2;


         auto value = vo;
         value += vo::Statistics{};
         value += vo;

         EXPECT_TRUE( value.invoked == 4);
         EXPECT_TRUE( value.min == 100us) << CASUAL_MAKE_NVP( vo);
         EXPECT_TRUE( value.max == 100us * 2);
         EXPECT_TRUE( value.total == 100us * 3 * 2);


      }


   } // transaction

} // casual

