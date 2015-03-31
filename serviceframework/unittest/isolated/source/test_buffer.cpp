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

         TEST( casual_sf_buffer, type_subtype)
         {
            Binary buffer( type::binary(), 1);


            auto type = buffer::type::get( buffer);

            EXPECT_TRUE( type == type::binary());
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



