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

      TEST( casual_common_string, adjacant_split__bla_bla_bla___gives_3_occurencies)
      {
         auto splittet = string::adjacent::split( "bla bla bla");

         EXPECT_TRUE( splittet.size() == 3) << "splittet.size(): " << splittet.size();
      }

      TEST( casual_common_string, split_oneword__gives_1_occurencies)
      {
         auto splittet = string::split( "oneword");

         EXPECT_TRUE( splittet.size() == 1);
      }

      TEST( casual_common_string, adjacent_split_bla___bla_bla__gives_3_occurencies)
      {
         auto splittet = string::adjacent::split( "bla    bla bla");

         EXPECT_TRUE( splittet.size() == 3);
      }

      TEST( casual_common_string, adjacent_split_bla___bla_bla__traling_ws__gives_3_occurencies)
      {
         auto splittet = string::adjacent::split( "  bla    bla bla  ");

         EXPECT_TRUE( splittet.size() == 3);
      }

      TEST( casual_common_string, split_casual_log_with_comma__gives_7_occurencies)
      {
         auto splittet = string::split( "error,warning,debug,information,casual.debug,casual.trace,casual.transaction", ',');

         ASSERT_TRUE( splittet.size() == 7) << "splittet.size(): " << splittet.size();
         EXPECT_TRUE( splittet.back() == "casual.transaction");
      }

      TEST( casual_common_string, join_two_strings_with_no_separator__gives_concatenated_string)
      {
         const auto result = string::join( std::vector<std::string>{ { "alpha"}, { "beta"}});
         EXPECT_TRUE( result == "alphabeta") << result;
      }

      TEST( casual_common_string, join_no_strings_with_no_separator__gives_empty_string)
      {
         const auto result = string::join( std::vector<std::string>{});
         EXPECT_TRUE( result.empty()) << result;
      }

      TEST( casual_common_string, join_three_strings_with_comma_separator__gives_concatenated_string)
      {
         const auto result = string::join( std::vector<std::string>{ { "alpha"}, { "beta"}, { "echo"}}, ",");
         EXPECT_TRUE( result == "alpha,beta,echo") << result;
      }

      TEST( casual_common_string, join_no_strings_with_comma_separator__gives_empty_string)
      {
         const auto result = string::join( std::vector<std::string>{}, ",");
         EXPECT_TRUE( result.empty()) << result;
      }

      TEST( casual_common_string, transform_normal_string_to_upper__gives_upper_string)
      {
         const auto result = string::upper( "Hello");
         EXPECT_TRUE( result == "HELLO") << result;
      }

      TEST( casual_common_string, transform_empty_string_to_upper__gives_empty_string)
      {
         const auto result = string::upper( "");
         EXPECT_TRUE( result.empty()) << result;
      }

      TEST( casual_common_string, transform_normal_string_to_lower__gives_lower_string)
      {
         const auto result = string::lower( "Hello");
         EXPECT_TRUE( result == "hello") << result;
      }

      TEST( casual_common_string, transform_empty_string_to_lower__gives_empty_string)
      {
         const auto result = string::lower( "");
         EXPECT_TRUE( result.empty()) << result;
      }



      TEST( casual_common_string, from_string_int)
      {
         EXPECT_TRUE( from_string< int>( "42") == 42);
      }

      TEST( casual_common_string, from_string_long)
      {
         EXPECT_TRUE( from_string< long>( "42") == 42);
      }

      TEST( casual_common_string, trim)
      {
         auto trimmed = string::trim( "  1 2 3 4 5   ");

         EXPECT_TRUE( trimmed.size() == 9);
         EXPECT_TRUE( trimmed.at( 0) == '1');
         EXPECT_TRUE( trimmed.at( 8) == '5');

         auto empty = string::trim( "   ");
         EXPECT_TRUE( empty.empty());

         auto hello = string::trim( "\n hello \t  ");
         EXPECT_TRUE( hello == "hello");
      }

      TEST( casual_common_type, type)
      {
         const auto type = type::name<long>();
         EXPECT_TRUE( type == "long") << type;
      }

      TEST( casual_common_string, integer_empty_expect_false)
      {
         EXPECT_FALSE( string::integer( ""));
      }

      TEST( casual_common_string, integer_42_expect_true)
      {
         EXPECT_TRUE( string::integer( "42"));
      }

      TEST( casual_common_string, integer_ABC_expect_false)
      {
         EXPECT_FALSE( string::integer( "ABC"));
      }

   }
}


