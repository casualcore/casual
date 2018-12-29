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
         EXPECT_TRUE( metric.total == platform::time::limit::zero());
         EXPECT_TRUE( metric.limit.min == platform::time::limit::zero());
         EXPECT_TRUE( metric.limit.max == platform::time::limit::zero());
      }

      TEST( common_metric, assign_one)
      {
         const auto second = std::chrono::seconds{ 1};
         Metric metric;
         metric += second;


         EXPECT_TRUE( metric.count == 1);
         EXPECT_TRUE( metric.total == second);
         EXPECT_TRUE( metric.limit.min == second);
         EXPECT_TRUE( metric.limit.max == second);
      }

      TEST( common_metric, assign_two)
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
   } // common
} // casual