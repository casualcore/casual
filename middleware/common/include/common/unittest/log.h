//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log.h"
#include "common/log/trace.h"
#include "common/string/compose.h"

#include "casual/assert.h"

#include <gtest/gtest.h>

namespace casual
{
   namespace common::unittest
   {
      //! Log with category 'casual.unittest'
      extern common::log::Stream log;

      //! Log with category 'casual.unittest.trace'
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
            auto test_info = casual::assertion( ::testing::UnitTest::GetInstance()->current_test_info());
            log::line( unittest::trace, "TEST( ", test_info->test_case_name(), ".", test_info->name(), ") - in");
         }
         ~Trace()
         {
            auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            log::line( unittest::trace, "TEST( ", test_info->test_case_name(), ".", test_info->name(), ") - out");
         }  

         template< typename... Ts>
         static void line( Ts&&... ts)
         {
            log::line( unittest::log, "TEST - ", std::forward< Ts>( ts)...);
         }

         template< typename S>
         static auto scope( S&& string) { return common::Trace{ std::forward< S>( string)};}

         template< typename... Ts>
         static auto compose( Ts&&... ts)
         {
            return string::compose( std::forward< Ts>( ts)...);
         }
      };

   } // common::unittest
} // casual


