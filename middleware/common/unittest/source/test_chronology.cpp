//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/chronology.h"

#include "common/code/casual.h"

#include <regex>

namespace casual
{
   namespace common
   {
      namespace chronology
      {
         TEST( common_chronology, from_string__invalid_unit)
         {
            unittest::Trace trace;

            EXPECT_CODE( from::string( "42ps");, code::casual::invalid_argument);
         }

         TEST( common_chronology, from_string__ms)
         {
            unittest::Trace trace;

            EXPECT_TRUE( from::string( "42ms") == std::chrono::milliseconds( 42));
            EXPECT_TRUE( from::string( "0ms") == std::chrono::milliseconds::zero());
            EXPECT_TRUE( from::string( "ms") == std::chrono::milliseconds::zero());
         }

         TEST( common_chronology, from_string__s)
         {
            unittest::Trace trace;

            EXPECT_TRUE( from::string( "42s") == std::chrono::seconds( 42));
            EXPECT_TRUE( from::string( "42") == std::chrono::seconds( 42));
            EXPECT_TRUE( from::string( "0s") == std::chrono::seconds::zero());
            EXPECT_TRUE( from::string( "s") == std::chrono::milliseconds::zero());
         }

         TEST( common_chronology, from_string__min)
         {
            unittest::Trace trace;

            EXPECT_TRUE( from::string( "42min") == std::chrono::minutes( 42));
            EXPECT_TRUE( from::string( "0min") == std::chrono::seconds::zero());
            EXPECT_TRUE( from::string( "min") == std::chrono::milliseconds::zero());
         }

         TEST( common_chronology, from_string__h)
         {
            unittest::Trace trace;

            EXPECT_TRUE( from::string( "42h") == std::chrono::hours( 42));
            EXPECT_TRUE( from::string( "0h") == std::chrono::seconds::zero());
            EXPECT_TRUE( from::string( "h") == std::chrono::milliseconds::zero());
         }

         TEST( common_chronology, from_string__us)
         {
            unittest::Trace trace;

            EXPECT_TRUE( from::string( "42us") == std::chrono::microseconds( 42));
            EXPECT_TRUE( from::string( "0us") == std::chrono::seconds::zero());
            EXPECT_TRUE( from::string( "us") == std::chrono::milliseconds::zero());
         }

         TEST( common_chronology, from_string__h_min_s)
         {
            unittest::Trace trace;

            EXPECT_TRUE( from::string( "1h+2min+12s") == std::chrono::seconds( 3600 + 120 + 12));
            EXPECT_TRUE( from::string( "1h + 2min + 12s") == std::chrono::seconds( 3600 + 120 + 12));
         }

         TEST( common_chronology, to_string__h_min_s)
         {
            unittest::Trace trace;

            EXPECT_TRUE( to::string( std::chrono::seconds( 3600 + 120 + 12)) == "1h + 2min + 12s") << to::string( std::chrono::seconds( 3600 + 120 + 12));
         }

         TEST( common_chronology, to_string_42ns)
         {
            unittest::Trace trace;

            auto duration = std::chrono::nanoseconds{ 42}; 

            EXPECT_TRUE( to::string( duration) == "42ns") << to::string( duration);
         }

         TEST( common_chronology, to_string__2h_40min_1ns)
         {
            unittest::Trace trace;

            auto duration = std::chrono::hours{ 2} + std::chrono::minutes{ 40} + std::chrono::nanoseconds{ 1}; 

            EXPECT_TRUE( to::string( duration) == "2h + 40min + 1ns") << to::string( duration);
         }

         TEST( common_chronology, iso8601_timestamp_format)
         {
            unittest::Trace trace;

            auto now = platform::time::clock::type::now();
            std::ostringstream out;
            out << now;

            // 2020-03-27T02:23:34.633337+05:30
            std::regex format{ R"(\d{4}-\d{2}-\d{2}T\d{2}\:\d{2}\:\d{2}\.\d{6}[+-]\d{2}\:\d{2})"};

            EXPECT_TRUE( std::regex_match( out.str(), format)) << "out: '" << out.str() << "'";
            
         }



      } // chronology
   } // common
} // casual
