//!
//! test_buffer.cpp
//!
//! Created on: Nov 25, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "xatmi.h"


namespace casual
{

   namespace buffer
   {


      TEST( casual_xatmi_buffer, STRING_allocate)
      {
         char* buffer = tpalloc( "STRING", 0, 2048);

         ASSERT_TRUE( buffer != 0);

         tpfree( buffer);

      }


      TEST( casual_xatmi_buffer, STRING_reallocate)
      {
         char* buffer = tpalloc( "STRING", 0, 2048);

         ASSERT_TRUE( buffer != 0);


         buffer = tprealloc( buffer, 4096);

         ASSERT_TRUE( buffer != 0);

         tpfree( buffer);


      }
   }
}


