//
// test_transcode.cpp
//
//  Created on: Dec 27, 2013
//      Author: Kristone
//




#include <gtest/gtest.h>

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
      TEST( casual_common_transcode, base64_encode)
      {
         EXPECT_TRUE( transcode::base64::encode( local::from_string( "")) == "");
         EXPECT_TRUE( transcode::base64::encode( local::from_string( "A")) == "QQ==");
         EXPECT_TRUE( transcode::base64::encode( local::from_string( "AB")) == "QUI=");
         EXPECT_TRUE( transcode::base64::encode( local::from_string( "ABC")) == "QUJD");
         EXPECT_TRUE( transcode::base64::encode( local::from_string( "ABCD")) == "QUJDRA==");
      }

      TEST( casual_common_transcode, base64_decode)
      {
         EXPECT_TRUE( transcode::base64::decode( "") == local::from_string( ""));
         EXPECT_TRUE( transcode::base64::decode( "QQ==") == local::from_string( "A"));
         EXPECT_TRUE( transcode::base64::decode( "QUI=") == local::from_string( "AB"));
         EXPECT_TRUE( transcode::base64::decode( "QUJD") == local::from_string( "ABC"));
         EXPECT_TRUE( transcode::base64::decode( "QUJDRA==") == local::from_string( "ABCD"));
      }

      TEST( casual_common_transcode, UT8_encode)
      {
         //const auto& closure = []( const unsigned char c){std::clog << static_cast<short>(c) << std::endl;};

         {
            const std::string source = { static_cast<std::string::value_type>(0xA4)};
            const std::string expect( u8"€");
            const std::string result = transcode::utf8::encode( source, "ISO-8859-15");
            EXPECT_TRUE( result == expect);
         }

      }

      TEST( casual_common_transcode, UT8_decode)
      {
         //const auto& closure = []( const unsigned char c){std::clog << static_cast<short>(c) << std::endl;};

         {
            const std::string source( u8"€");
            const std::string expect = { static_cast<std::string::value_type>(0xA4)};
            const std::string result = transcode::utf8::decode( source, "ISO-8859-15");
            EXPECT_TRUE( result == expect);
         }

      }

      TEST( casual_common_transcode, UT8_encode_exotic_characters)
      {
         const std::string source( u8"Bängen Trålar");
         const std::string result = transcode::utf8::encode( source);
         EXPECT_TRUE( result == source);
      }



      /*
      TEST( casual_common_transcode, UT8_encode_decode)
      {
         //const auto& closure = []( const unsigned char c){std::clog << static_cast<short>(c) << std::endl;};

         const std::string source( u8"Bängen Trålar");
         //const std::string target = transcode::utf8::encode( source, "ISO-8859-1");
         //const std::string result = transcode::utf8::decode( target, "ISO-8859-1");
         const std::string target = transcode::utf8::encode( source, "en_US.utf88");
         const std::string result = transcode::utf8::decode( target, "en_US.utf88");

         EXPECT_TRUE( source == result);

      }
      */

      TEST( casual_common_transcode, hex_encode)
      {
         std::vector< std::uint8_t> binary{ 255, 0, 240, 10};

         EXPECT_TRUE( transcode::hex::encode( binary) == "ff00f00a") << "hex: " << transcode::hex::encode( binary);
      }

   }
}


