//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/metric.h"

namespace casual
{
   namespace common
   {
      
      TEST( common_metric, default_ctor)
      {
         Metric metric;

         EXPECT_TRUE( metric.count == 0);
         EXPECT_TRUE( metric.total == platform::time::unit::zero());
         EXPECT_TRUE( metric.limit.min == platform::time::unit::zero());
         EXPECT_TRUE( metric.limit.max == platform::time::unit::zero());
      }

      TEST( common_metric, compound_assign_1_duration)
      {
         const auto second = std::chrono::seconds{ 1};
         Metric metric;
         metric += second;


         EXPECT_TRUE( metric.count == 1);
         EXPECT_TRUE( metric.total == second);
         EXPECT_TRUE( metric.limit.min == second);
         EXPECT_TRUE( metric.limit.max == second);
      }

      TEST( common_metric, compound_assign_2_duration)
      {
         const auto max = std::chrono::seconds{ 2};
         const auto min = std::chrono::milliseconds{ 234};
         Metric metric;
         metric += max;
         metric += min;


         EXPECT_TRUE( metric.count == 2);
         EXPECT_TRUE( metric.total == min + max);
         EXPECT_TRUE( metric.limit.min == min);
         EXPECT_TRUE( metric.limit.max == max);
      }

      namespace local
      {
         namespace
         {
            auto create_metric = []( auto first, auto second)
            {
               Metric metric;
               metric += first;
               metric += second;
               return metric;
            };
         } // <unnamed>
      } // local

      TEST( common_metric, compound_assign_other)
      {
         using ms = std::chrono::milliseconds;
         using s = std::chrono::seconds;

         auto metric = local::create_metric( ms{ 234}, s{ 2});
         metric += local::create_metric( ms{ 42}, s{ 1});

         EXPECT_TRUE( metric.count == 4);
         EXPECT_TRUE( metric.total == ms{ 234} + s{ 2} + ms{ 42} + s{ 1});
         EXPECT_TRUE( metric.limit.min == ms{ 42});
         EXPECT_TRUE( metric.limit.max == s{ 2});
      }

      TEST( common_metric, add)
      {
         using ms = std::chrono::milliseconds;
         using s = std::chrono::seconds;

         auto metric = local::create_metric( ms{ 234}, s{ 2}) 
            + local::create_metric( ms{ 14}, s{ 1})
            + local::create_metric( ms{ 42}, s{ 3});


         EXPECT_TRUE( metric.count == 6);
         EXPECT_TRUE( metric.total == ms{ 234} + s{ 2} + ms{ 14} + s{ 1} + ms{ 42} + s{ 3});
         EXPECT_TRUE( metric.limit.min ==  ms{ 14});
         EXPECT_TRUE( metric.limit.max == s{ 3});
      }

      TEST( common_metric, equality)
      {
         using ms = std::chrono::milliseconds;
         using s = std::chrono::seconds;

         auto metric = local::create_metric( ms{ 234}, s{ 2}) 
            + local::create_metric( ms{ 14}, s{ 1})
            + local::create_metric( ms{ 42}, s{ 3});

         auto copy = metric;


         EXPECT_TRUE( metric == copy);
         EXPECT_TRUE( metric != local::create_metric( ms{ 14}, s{ 1}));
      }
   } // common
} // casual