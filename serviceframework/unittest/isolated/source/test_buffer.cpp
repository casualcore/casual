//!
//! test_buffer.cpp
//!
//! Created on: Dec 23, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "sf/buffer.h"
#include "xatmi.h"



namespace casual
{
   namespace sf
   {
      namespace buffer
      {

         TEST( casual_sf_buffer, X_OCTET_type_subtype)
         {
            X_Octet buffer( "YAML");


            Type type = buffer.type();

            EXPECT_TRUE( type.name == "X_OCTET");
            EXPECT_TRUE( type.subname == "YAML");

         }

      }
   }
}



