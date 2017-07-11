//!
//! casual
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

         auto us100 = std::chrono::microseconds{ 100};

         statistics.start( now);
         statistics.end( now + us100);

         EXPECT_TRUE( statistics.invoked == 1);
         EXPECT_TRUE( statistics.min == us100);
         EXPECT_TRUE( statistics.max == us100);
         EXPECT_TRUE( statistics.total == us100);

      }

      TEST( casual_transaction_admin, transform_statistics)
      {
         common::unittest::Trace trace;

         state::Statistics statistics;

         auto now = common::platform::time::clock::type::now();

         auto us100 = std::chrono::microseconds{ 100};

         statistics.start( now);
         statistics.end( now + us100);

         auto vo = transform::Statistics{}( statistics);

         EXPECT_TRUE( vo.invoked == 1);
         EXPECT_TRUE( vo.min == us100);
         EXPECT_TRUE( vo.max == us100);
         EXPECT_TRUE( vo.total == us100);

      }

      TEST( casual_transaction_admin, vo_statistics__addition_assignment__rhs_defualt)
      {
         common::unittest::Trace trace;

         state::Statistics statistics;

         auto now = common::platform::time::clock::type::now();

         auto us100 = std::chrono::microseconds{ 100};

         statistics.start( now);
         statistics.end( now + us100);

         auto vo = transform::Statistics{}( statistics);
         vo += vo::Statistics{};

         EXPECT_TRUE( vo.invoked == 1);
         EXPECT_TRUE( vo.min == us100);
         EXPECT_TRUE( vo.max == us100);
         EXPECT_TRUE( vo.total == us100);

      }

      TEST( casual_transaction_admin, vo_statistics__addition_assignment__lhs_defualt)
      {
         common::unittest::Trace trace;

         state::Statistics statistics;

         auto now = common::platform::time::clock::type::now();

         auto us100 = std::chrono::microseconds{ 100};

         statistics.start( now);
         statistics.end( now + us100);

         vo::Statistics vo;
         vo += transform::Statistics{}( statistics);

         EXPECT_TRUE( vo.invoked == 1);
         EXPECT_TRUE( vo.min == us100) << CASUAL_MAKE_NVP( vo);
         EXPECT_TRUE( vo.max == us100);
         EXPECT_TRUE( vo.total == us100);

      }

      TEST( casual_transaction_admin, vo_statistics__addition_assignment__rhs_defualt_2x)
      {
         common::unittest::Trace trace;

         vo::Statistics vo;

         auto us100 = std::chrono::microseconds{ 100};

         vo.min = us100;
         vo.max = us100 + us100;
         vo.total = us100 * 3;
         vo.invoked = 2;


         auto value = vo;
         value += vo::Statistics{};
         value += vo;

         EXPECT_TRUE( value.invoked == 4);
         EXPECT_TRUE( value.min == us100) << CASUAL_MAKE_NVP( vo);
         EXPECT_TRUE( value.max == us100 * 2);
         EXPECT_TRUE( value.total == us100 * 3 * 2);


      }


   } // transaction

} // casual

