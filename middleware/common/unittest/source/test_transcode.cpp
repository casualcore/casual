//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <common/unittest.h>

#include "common/transcode.h"

#include <algorithm>

namespace casual
{

   namespace
   {
      namespace local
      {
         std::vector<char> from_string( const std::string& string)
         {
            return std::vector<char>( string.begin(), string.end());
         }
      }
   }

   namespace common
   {
      TEST( casual_common_transcode_base64, encode)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( transcode::base64::encode( std::string( "")) == "");
         EXPECT_TRUE( transcode::base64::encode( std::string( "A")) == "QQ==");
         EXPECT_TRUE( transcode::base64::encode( std::string( "AB")) == "QUI=");
         EXPECT_TRUE( transcode::base64::encode( std::string( "ABC")) == "QUJD");
         EXPECT_TRUE( transcode::base64::encode( std::string( "ABCD")) == "QUJDRA==");
      }

      TEST( casual_common_transcode_base64, decode)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( transcode::base64::decode( "") == local::from_string( ""));
         EXPECT_TRUE( transcode::base64::decode( "QQ==") == local::from_string( "A"));
         EXPECT_TRUE( transcode::base64::decode( "QUI=") == local::from_string( "AB"));
         EXPECT_TRUE( transcode::base64::decode( "QUJD") == local::from_string( "ABC"));
         EXPECT_TRUE( transcode::base64::decode( "QUJDRA==") == local::from_string( "ABCD"));
      }

      TEST( casual_common_transcode_utf8, test_existene_of_bogus_codeset__expecting_false)
      {
         common::unittest::Trace trace;

         EXPECT_FALSE( transcode::utf8::exist( "casual"));
      }

      TEST( casual_common_transcode_utf8, test_existene_of_utf8_codeset__expecting_true)
      {
         common::unittest::Trace trace;

         // this codeset should exist (or if not we're screwed)
         EXPECT_TRUE( transcode::utf8::exist( "UTF-8") || transcode::utf8::exist( "UTF8"));
      }

      TEST( casual_common_transcode_utf8, encode_euro_sign)
      {
         common::unittest::Trace trace;

         if( transcode::utf8::exist( "ISO-8859-15"))
         {
            const std::string source = { static_cast<std::string::value_type>(0xA4)};
            const std::string expect( u8"€");
            const std::string result = transcode::utf8::encode( source, "ISO-8859-15");
            EXPECT_TRUE( result == expect);
         }
         else
         {
            GTEST_LOG_(WARNING);
         }
      }

      TEST( casual_common_transcode_utf8, decode_euro_sign)
      {
         common::unittest::Trace trace;

         if( transcode::utf8::exist( "ISO-8859-15"))
         {
            const std::string source( u8"\u20AC");
            const std::string expect = { static_cast<std::string::value_type>(0xA4)};
            const std::string result = transcode::utf8::decode( source, "ISO-8859-15");
            EXPECT_TRUE( result == expect);
         }
         else
         {
            GTEST_LOG_(WARNING);
         }
      }

      TEST( casual_common_transcode_utf8, UTF8_encode_exotic_characters)
      {
         common::unittest::Trace trace;

         if( transcode::utf8::exist( "ISO-8859-1"))
         {
            const std::string source{ static_cast<std::string::value_type>(0xE5), static_cast<std::string::value_type>(0xE4), static_cast<std::string::value_type>(0xF6)};
            const std::string expect{ u8"åäö"};
            const std::string result = transcode::utf8::encode( source, "ISO-8859-1");
            EXPECT_TRUE( result == expect);
         }
         else
         {
            GTEST_LOG_(WARNING);
         }
      }

      TEST( casual_common_transcode_hex, encode)
      {
         common::unittest::Trace trace;

         std::vector< std::uint8_t> binary{ 255, 0, 240, 10};

         EXPECT_TRUE( transcode::hex::encode( binary) == "ff00f00a") << "hex: " << transcode::hex::encode( binary);
      }

      TEST( casual_common_transcode_hex, decode)
      {
         common::unittest::Trace trace;

         auto binary = transcode::hex::decode( "ff00f00a");

         EXPECT_TRUE( transcode::hex::encode( binary) == "ff00f00a") << "hex: " << transcode::hex::encode( binary);
      }

   }
}


