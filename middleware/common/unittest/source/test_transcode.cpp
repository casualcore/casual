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

   namespace common
   {

      namespace local
      {
         namespace 
         {

            platform::binary::type string_to_binary( std::string_view value)
            {
               auto span = binary::span::make( value);
               return platform::binary::type( std::begin( span), std::end( span));
            }

         } // <unnamed>
      } // local

      TEST( casual_common_transcode_base64, encode)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( transcode::base64::encode( local::string_to_binary( "")) == "");
         EXPECT_TRUE( transcode::base64::encode( local::string_to_binary( "A")) == "QQ==");
         EXPECT_TRUE( transcode::base64::encode( local::string_to_binary( "AB")) == "QUI=");
         EXPECT_TRUE( transcode::base64::encode( local::string_to_binary( "ABC")) == "QUJD");
         EXPECT_TRUE( transcode::base64::encode( local::string_to_binary( "ABCD")) == "QUJDRA==");
      }

      TEST( casual_common_transcode_base64, decode)
      {
         common::unittest::Trace trace;

         using namespace std::string_view_literals;

         EXPECT_TRUE( transcode::base64::decode( ""sv) == local::string_to_binary( ""));
         EXPECT_TRUE( transcode::base64::decode( "QQ=="sv) == local::string_to_binary( "A"));
         EXPECT_TRUE( transcode::base64::decode( "QUI="sv) == local::string_to_binary( "AB"));
         EXPECT_TRUE( transcode::base64::decode( "QUJD"sv) == local::string_to_binary( "ABC"));
         EXPECT_TRUE( transcode::base64::decode( "QUJDRA=="sv) == local::string_to_binary( "ABCD"));
      }

      TEST( casual_common_transcode_utf8, test_existene_of_bogus_codeset__expecting_false)
      {
         common::unittest::Trace trace;

         EXPECT_FALSE( transcode::utf8::exist( "casual"));
      }

      TEST( casual_common_transcode_utf8, test_existene_of_utf8_codeset__expecting_true)
      {
         common::unittest::Trace trace;

         // this codeset should exist (or we're screwed)
         EXPECT_TRUE( transcode::utf8::exist( "UTF-8") || transcode::utf8::exist( "UTF8"));
      }

      TEST( casual_common_transcode_utf8, encode_euro_sign)
      {
         common::unittest::Trace trace;

         if( transcode::utf8::exist( "ISO-8859-15"))
         {
            const std::string source = { static_cast<std::string::value_type>(0xA4)};
            const std::string expect( "€");
            const std::string result = transcode::utf8::string::encode( source, "ISO-8859-15");
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
            const std::string source( "\u20AC");
            const std::string expect = { static_cast<std::string::value_type>(0xA4)};
            const std::string result = transcode::utf8::string::decode( source, "ISO-8859-15");
            EXPECT_TRUE( result == expect);
         }
         else
         {
            GTEST_LOG_(WARNING);
         }
      }


      TEST( casual_common_transcode_utf8, transcode_ws)
      {
         common::unittest::Trace trace;
         EXPECT_TRUE( transcode::utf8::string::encode( " ") == " ");
         EXPECT_TRUE( transcode::utf8::string::decode( " ") == " ");
         EXPECT_TRUE( transcode::utf8::string::decode( transcode::utf8::string::encode( " ")) == " ");
      }

      TEST( casual_common_transcode_utf8, UTF8_encode_exotic_characters)
      {
         common::unittest::Trace trace;

         if( transcode::utf8::exist( "ISO-8859-1"))
         {
            const std::string source{ static_cast<std::string::value_type>(0xE5), static_cast<std::string::value_type>(0xE4), static_cast<std::string::value_type>(0xF6)};
            const std::string expect{ "åäö"};
            const std::string result = transcode::utf8::string::encode( source, "ISO-8859-1");
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

         platform::binary::type binary{ std::byte{ 255}, std::byte{ 0}, std::byte{ 240}, std::byte{ 10}};

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


