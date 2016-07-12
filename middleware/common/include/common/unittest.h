//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_


#include "common/trace.h"
#include "common/log.h"


#include <gtest/gtest.h>

namespace casual
{
   namespace common
   {
      namespace unittest
      {

         namespace detail
         {
            struct Trace
            {
               Trace()
               {
                  auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();

                  log::trace << "TEST( " << test_info->test_case_name() << ", " << test_info->name() << ") - in\n";
               }

               ~Trace()
               {
                  auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();

                  log::trace << "TEST( " << test_info->test_case_name() << ", " << test_info->name() << ") - out\n";
               }
            };

         } // detail
      } // unittest
   } // common
} // casual

#define CASUAL_UNITTEST_TRACE() \
   casual::common::unittest::detail::Trace casual_unittest_trace

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_
