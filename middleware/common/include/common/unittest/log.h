//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log.h"
#include "common/log/trace.h"

#include <gtest/gtest.h>

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         //! Log with category 'casual.mockup'
         extern common::log::Stream log;

         //! Log with category 'casual.mockup.trace'
         extern common::log::Stream trace;


         namespace clean
         {
            //! tries to clean all signals when scope is entered and exited.
            //! also clears the default inbound ipc device.
            struct Scope
            {
               Scope();
               ~Scope();
            };

         } // clean


         struct Trace : clean::Scope
         {
            Trace()
            {
               auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();
               log::line( unittest::log, "TEST( ", test_info->test_case_name(), ".", test_info->name(), ") - in");
            }
            ~Trace()
            {
               auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();
               log::line( unittest::log, "TEST( ", test_info->test_case_name(), ".", test_info->name(), ") - out");
            }
         };
      } // unittest
   } // common
} // casual


