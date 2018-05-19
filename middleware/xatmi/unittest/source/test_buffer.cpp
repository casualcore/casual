//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <gtest/gtest.h>

#include "xatmi.h"

#include <array>


namespace casual
{

   namespace buffer
   {


      TEST( casual_xatmi_buffer, X_OCTET_allocate)
      {
         char* buffer = tpalloc( X_OCTET, 0, 2048);

         ASSERT_TRUE( buffer != 0);

         tpfree( buffer);

      }


      TEST( casual_xatmi_buffer, X_OCTET_reallocate)
      {
         char* buffer = tpalloc( X_OCTET, 0, 2048);

         ASSERT_TRUE( buffer != 0);


         buffer = tprealloc( buffer, 4096);

         ASSERT_TRUE( buffer != 0);

         tpfree( buffer);


      }

      TEST( casual_xatmi_buffer, X_OCTET_tptypes)
      {
         char* buffer = tpalloc( X_OCTET, 0, 666);
         ASSERT_TRUE( buffer != 0);

         std::array< char, 8> type;
         std::array< char, 16> subtype;

         EXPECT_TRUE( tptypes( buffer, type.data(), subtype.data()) == 666);
         EXPECT_TRUE( std::string( type.data()) == "X_OCTET") << "type.data(): " << type.data();
         EXPECT_TRUE( std::string( subtype.data()) == "") << "subtype.data(): " << subtype.data();


         //tpfree( buffer);

      }
   }
}


