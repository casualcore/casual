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

namespace casual
{
   namespace common
   {

      TEST( casual_common_signal, scope_timeout)
      {
         //
         // Start from a clean sheet
         //
         signal::clear();

         EXPECT_NO_THROW( signal::handle());

         signal::timer::Scoped timer{ std::chrono::milliseconds{ 1}};

         process::sleep( std::chrono::milliseconds{ 2});

         EXPECT_THROW(
         {
            signal::handle();

         }, exception::signal::Timeout);
      }

      TEST( casual_common_signal, nested_timeout)
      {
         //
         // Start from a clean sheet
         //
         signal::clear();

         EXPECT_NO_THROW( signal::handle());

         signal::timer::Scoped timer1{ std::chrono::milliseconds{ 5}};

         {
            signal::timer::Scoped timer2{ std::chrono::milliseconds{ 1}};

            process::sleep( std::chrono::milliseconds{ 2});

            EXPECT_THROW(
            {
               signal::handle();
            }, exception::signal::Timeout);
         }

         process::sleep( std::chrono::milliseconds{ 4});

         EXPECT_THROW(
         {
            signal::handle();
         }, exception::signal::Timeout);
      }

   } // common

} // casual
