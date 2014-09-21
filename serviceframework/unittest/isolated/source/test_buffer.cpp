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
            X_Octet buffer( "yaml");


            Type type = buffer::type( buffer);

            EXPECT_TRUE( type.name == "X_OCTET");
            EXPECT_TRUE( type.subname == "yaml");

         }

         TEST( casual_sf_buffer, test)
         {
            binary::Stream input;

            input << std::string{ "bla"};

            binary::Stream output = std::move( input);

            std::string out;
            output >> out;

            EXPECT_TRUE( out == "bla");



         }

      }
   }
}



