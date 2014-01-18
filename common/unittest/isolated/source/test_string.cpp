//!
//! test_string.cpp
//!
//! Created on: Jun 8, 2013
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "common/string.h"

namespace casual
{

   namespace common
   {
      TEST( casual_common_string, split_bla_bla_bla__gives_3_occurencies)
      {
         auto splittet = string::split( "bla bla bla");

         EXPECT_TRUE( splittet.size() == 3);
      }

      TEST( casual_common_string, split_oneword__gives_1_occurencies)
      {
         auto splittet = string::split( "oneword");

         EXPECT_TRUE( splittet.size() == 1);
      }

      TEST( casual_common_string, split_bla___bla_bla__gives_3_occurencies)
      {
         auto splittet = string::split( "bla    bla bla");

         EXPECT_TRUE( splittet.size() == 3);
      }

      TEST( casual_common_string, split_bla___bla_bla__traling_ws__gives_3_occurencies)
      {
         auto splittet = string::split( "  bla    bla bla  ");

         EXPECT_TRUE( splittet.size() == 3);
      }

      TEST( casual_common_string, split_casual_log_with_comma__gives_7_occurencies)
      {
         auto splittet = string::split( "error,warning,debug,information,casual.debug,casual.trace,casual.transaction", ',');

         ASSERT_TRUE( splittet.size() == 7);
         EXPECT_TRUE( splittet.back() == "casual.transaction");
      }

      TEST( casual_common_string, from_string_int)
      {
         EXPECT_TRUE( from_string< int>( "42") == 42);
      }

      TEST( casual_common_string, from_string_long)
      {
         EXPECT_TRUE( from_string< long>( "42") == 42);
      }

   }
}


