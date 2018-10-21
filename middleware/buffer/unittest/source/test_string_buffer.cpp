//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>

#include "buffer/string.h"
#include "xatmi.h"

#include <cstring>


TEST( casual_string_buffer, allocate_with_normal_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_STRING, "", 32);

   ASSERT_TRUE( buffer != nullptr);

   EXPECT_TRUE( tptypes( buffer, nullptr, nullptr) == 32);

   EXPECT_TRUE( std::strlen( buffer) == 0);

   tpfree( buffer);
}

TEST( casual_string_buffer, raw_set_to_allocated_buffer__expecting_correct_parse)
{
   auto buffer = tpalloc( CASUAL_STRING, "", 16);

   const char* source = "Hello";
   std::strncpy( buffer, source, 16);

   const char* value = "";

   EXPECT_FALSE( casual_string_get( buffer, &value));

   EXPECT_STREQ( source, value);

   tpfree( buffer);
}

TEST( casual_string_buffer, set_to_small_allocated_buffer__expecting_resized_buffer)
{
   auto buffer = tpalloc( CASUAL_STRING, "", 4);

   const char* source = "Hello Casual";

   EXPECT_FALSE( casual_string_set( &buffer, source));

   const char* value = "";

   EXPECT_FALSE( casual_string_get( buffer, &value));

   EXPECT_STREQ( source, value);

   EXPECT_EQ( static_cast<std::size_t>(tptypes( buffer, nullptr, nullptr)), std::strlen( source) + 1);

   tpfree( buffer);
}

TEST( casual_string_buffer, destroy_buffer__expecting_out_of_bounds)
{
   auto buffer = tpalloc( CASUAL_STRING, "", 4);

   const char* source = "Hello Casual";

   EXPECT_FALSE( casual_string_set( &buffer, source));

   buffer[std::strlen( source)] = 'x';

   const char* value = "";

   EXPECT_TRUE( casual_string_get( buffer, &value) == CASUAL_STRING_OUT_OF_BOUNDS);

   tpfree( buffer);
}




