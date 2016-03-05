//!
//! test_signal.cpp
//!
//! Created on: Aug 10, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/signal.h"
#include "common/process.h"
#include "common/exception.h"
#include "common/trace.h"

namespace casual
{
   namespace common
   {

      TEST( casual_common_signal, scope_timeout)
      {
         Trace trace{ "TEST( casual_common_signal, scope_timeout)", log::internal::trace};

         //
         // Start from a clean sheet
         //
         signal::clear();

         EXPECT_NO_THROW( signal::handle());

         signal::timer::Scoped timer{ std::chrono::milliseconds{ 1}};

         EXPECT_THROW(
         {
            process::sleep( std::chrono::milliseconds{ 2});

         }, exception::signal::Timeout);
      }

      TEST( casual_common_signal, nested_timeout)
      {
         Trace trace{ "TEST( casual_common_signal, nested_timeout)", log::internal::trace};

         //
         // Start from a clean sheet
         //
         signal::clear();

         EXPECT_NO_THROW( signal::handle());

         signal::timer::Scoped timer1{ std::chrono::milliseconds{ 5}};

         {
            signal::timer::Scoped timer2{ std::chrono::milliseconds{ 1}};


            EXPECT_THROW(
            {
               process::sleep( std::chrono::milliseconds{ 10});
            }, exception::signal::Timeout);
         }

         EXPECT_THROW(
         {
            process::sleep( std::chrono::milliseconds{ 10});
         }, exception::signal::Timeout);
      }

   } // common

} // casual
