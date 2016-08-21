//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_

#include "common/signal.h"
#include "common/log.h"

#include <gtest/gtest.h>

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace clean
         {
            //!
            //! tries to clean all signals when scope is entered and exited
            //!
            struct Scope
            {
               Scope() { signal::clear();}
               ~Scope() { signal::clear();}
            };

         } // clean

         struct Trace : clean::Scope
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

      } // unittest
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_
