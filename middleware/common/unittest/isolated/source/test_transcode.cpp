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
         EXPECT_TRUE( transcode::base64::encode( std::string( "")) == "");
         EXPECT_TRUE( transcode::base64::encode( std::string( "A")) == "QQ==");
         EXPECT_TRUE( transcode::base64::encode( std::string( "AB")) == "QUI=");
         EXPECT_TRUE( transcode::base64::encode( std::string( "ABC")) == "QUJD");
         EXPECT_TRUE( transcode::base64::encode( std::string( "ABCD")) == "QUJDRA==");
      }

      TEST( casual_common_transcode, base64_decode)
      {
         EXPECT_TRUE( transcode::base64::decode( "") == local::from_string( ""));
         EXPECT_TRUE( transcode::base64::decode( "QQ==") == local::from_string( "A"));
         EXPECT_TRUE( transcode::base64::decode( "QUI=") == local::from_string( "AB"));
         EXPECT_TRUE( transcode::base64::decode( "QUJD") == local::from_string( "ABC"));
         EXPECT_TRUE( transcode::base64::decode( "QUJDRA==") == local::from_string( "ABCD"));
      }

      // TODO: Enable these tests whenever iconv -l shows that ISO-8859-15 is present on Suse
      TEST( DISABLED_casual_common_transcode, UT8_encode_euro_sign)
      {
         //const auto& closure = []( const unsigned char c){std::clog << static_cast<short>(c) << std::endl;};

         {
            const std::string source = { static_cast<std::string::value_type>(0xA4)};
            const std::string expect( u8"€");
            const std::string result = transcode::utf8::encode( source, "ISO-8859-15");
            EXPECT_TRUE( result == expect);
         }

      }

      // TODO: Enable these tests whenever iconv -l shows that ISO-8859-15 is present on Suse
      TEST( DISABLED_casual_common_transcode, UT8_decode_euro_sign)
      {
         //const auto& closure = []( const unsigned char c){std::clog << static_cast<short>(c) << std::endl;};

         // €
         const std::string source( u8"\u20AC");
         const std::string expect = { static_cast<std::string::value_type>(0xA4)};
         const std::string result = transcode::utf8::decode( source, "ISO-8859-15");
         EXPECT_TRUE( result == expect);

      }

      // TODO: gives warning from clang and gives failure on OSX with locale "UTF-8"
      TEST( DISABLED_casual_common_transcode, UTF8_encode_exotic_characters)
      {
         const std::string source{ static_cast<std::string::value_type>(0xE5), static_cast<std::string::value_type>(0xE4), static_cast<std::string::value_type>(0xF6)};
         const std::string expect{ u8"åäö"};
         const std::string result = transcode::utf8::encode( source, "ISO-8859-1");
         EXPECT_TRUE( result == expect);
      }


      TEST( casual_common_transcode, hex_encode)
      {
         std::vector< std::uint8_t> binary{ 255, 0, 240, 10};

         EXPECT_TRUE( transcode::hex::encode( binary) == "ff00f00a") << "hex: " << transcode::hex::encode( binary);
      }

      TEST( casual_common_transcode, hex_decode)
      {
         auto binary = transcode::hex::decode( "ff00f00a");

         EXPECT_TRUE( transcode::hex::encode( binary) == "ff00f00a") << "hex: " << transcode::hex::encode( binary);
      }

   }
}


