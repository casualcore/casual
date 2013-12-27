//
// test_transcoding.cpp
//
//  Created on: Dec 27, 2013
//      Author: Kristone
//




#include <gtest/gtest.h>

#include "common/transcoding.h"

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
      TEST( casual_common_transcoding, Base64_encode)
      {
         EXPECT_TRUE( transcoding::Base64::encode( local::from_string( "")) == "");
         EXPECT_TRUE( transcoding::Base64::encode( local::from_string( "A")) == "QQ==");
         EXPECT_TRUE( transcoding::Base64::encode( local::from_string( "AB")) == "QUI=");
         EXPECT_TRUE( transcoding::Base64::encode( local::from_string( "ABC")) == "QUJD");
         EXPECT_TRUE( transcoding::Base64::encode( local::from_string( "ABCD")) == "QUJDRA==");
      }

      TEST( casual_common_transcoding, Base64_decode)
      {
         EXPECT_TRUE( transcoding::Base64::decode( "") == local::from_string( ""));
         EXPECT_TRUE( transcoding::Base64::decode( "QQ==") == local::from_string( "A"));
         EXPECT_TRUE( transcoding::Base64::decode( "QUI=") == local::from_string( "AB"));
         EXPECT_TRUE( transcoding::Base64::decode( "QUJD") == local::from_string( "ABC"));
         EXPECT_TRUE( transcoding::Base64::decode( "QUJDRA==") == local::from_string( "ABCD"));
      }

   }
}


