//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <gtest/gtest.h>

#include "buffer/order.h"
#include "xatmi.h"


#include <string>




TEST( casual_order_buffer, allocate_with_enough_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 512);
   ASSERT_TRUE( buffer != nullptr);

   tpfree( buffer);
}

TEST( casual_order_buffer, allocate_with_zero_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 0);
   ASSERT_TRUE( buffer != nullptr);

   long used = 0;
   EXPECT_TRUE( casual_order_explore_buffer( buffer, nullptr, &used, nullptr) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( used == 0);

   tpfree( buffer);
}


TEST( casual_order_buffer, reallocate_with_bigger_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 64);
   EXPECT_TRUE( buffer != nullptr);

   buffer = tprealloc( buffer, 128);

   EXPECT_TRUE( buffer != nullptr);

   tpfree( buffer);
}

TEST( casual_order_buffer, reallocate_with_smaller_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 128);
   EXPECT_TRUE( buffer != nullptr);

   buffer = tprealloc( buffer, 64);

   EXPECT_TRUE( buffer != nullptr);

   tpfree( buffer);
}


TEST( casual_order_buffer, add_and_get)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 1024);
   ASSERT_TRUE( buffer != nullptr);

   ASSERT_TRUE( casual_order_add_bool( &buffer, false) == CASUAL_ORDER_SUCCESS);
   ASSERT_TRUE( casual_order_add_char( &buffer, 'a') == CASUAL_ORDER_SUCCESS);
   ASSERT_TRUE( casual_order_add_short( &buffer, 123) == CASUAL_ORDER_SUCCESS);
   ASSERT_TRUE( casual_order_add_long( &buffer, 654321) == CASUAL_ORDER_SUCCESS);
   ASSERT_TRUE( casual_order_add_float( &buffer, 3.14) == CASUAL_ORDER_SUCCESS);
   ASSERT_TRUE( casual_order_add_double( &buffer, 987.654) == CASUAL_ORDER_SUCCESS);
   ASSERT_TRUE( casual_order_add_string( &buffer, "Hello!") == CASUAL_ORDER_SUCCESS);
   ASSERT_TRUE( casual_order_add_binary( &buffer, "Some Data", 9) == CASUAL_ORDER_SUCCESS);


   {
      bool boolean;
      const auto result = casual_order_get_bool( buffer, &boolean);
      EXPECT_TRUE( result == CASUAL_ORDER_SUCCESS) << result;
      EXPECT_TRUE( boolean == false);
   }


   {
      char character;
      const auto result = casual_order_get_char( buffer, &character);
      EXPECT_TRUE( result == CASUAL_ORDER_SUCCESS) << result;
      EXPECT_TRUE( character == 'a');
   }

   {
      short integer;
      const auto result = casual_order_get_short( buffer, &integer);
      EXPECT_TRUE( result == CASUAL_ORDER_SUCCESS) << result;
      EXPECT_TRUE( integer == 123);
   }

   {
      long integer;
      const auto result = casual_order_get_long( buffer, &integer);
      EXPECT_TRUE( result == CASUAL_ORDER_SUCCESS) << result;
      EXPECT_TRUE( integer == 654321);
   }

   {
      float decimal;
      const auto result = casual_order_get_float( buffer, &decimal);
      EXPECT_TRUE( result == CASUAL_ORDER_SUCCESS) << result;
      EXPECT_TRUE( decimal > 3.1 && decimal < 3.2);
   }

   {
      double decimal;
      const auto result = casual_order_get_double( buffer, &decimal);
      EXPECT_TRUE( result == CASUAL_ORDER_SUCCESS) << result;
      EXPECT_TRUE( decimal > 987.6 && decimal < 987.7);
   }

   {
      const char* string = nullptr;
      const auto result = casual_order_get_string( buffer, &string);
      EXPECT_TRUE( result == CASUAL_ORDER_SUCCESS) << result;
      EXPECT_STREQ( string, "Hello!") << string;
   }



   {
      const char* binary = nullptr;
      long size;
      const auto result = casual_order_get_binary( buffer, &binary, &size);
      EXPECT_TRUE( result == CASUAL_ORDER_SUCCESS) << result;
      EXPECT_TRUE( std::string( binary, size) == "Some Data") << std::string( binary, size) << binary << " " << size << std::endl;
      EXPECT_TRUE( size == 9);
   }

   {
      bool none;
      const auto result = casual_order_get_bool( buffer, &none);
      EXPECT_TRUE( result == CASUAL_ORDER_OUT_OF_BOUNDS) << result;
   }

   tpfree( buffer);
}

TEST( casual_order_buffer, detect_reallocation)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 64);
   ASSERT_TRUE( buffer != nullptr);


   EXPECT_FALSE( casual_order_add_long( &buffer, 123456));
   EXPECT_FALSE( casual_order_add_string( &buffer, std::string( 'a', 64).c_str()));

   EXPECT_TRUE( tptypes( buffer, nullptr, nullptr) > 63);

   tpfree( buffer);
}

TEST( casual_order_buffer, detect_invalid_buffer)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 64);
   ASSERT_TRUE( buffer != nullptr);


   EXPECT_TRUE( casual_order_add_long( &buffer, 123456) == CASUAL_ORDER_SUCCESS);
   long integer;
   EXPECT_TRUE( casual_order_get_long( buffer, &integer) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( casual_order_get_long( buffer, &integer) == CASUAL_ORDER_OUT_OF_BOUNDS);

   tpfree( buffer);
}

TEST( casual_order_buffer, add_binary_data_with_negative_size__expecting_invalid_argument)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 0);
   ASSERT_TRUE( buffer != nullptr);

   EXPECT_TRUE( casual_order_add_binary( &buffer, "some data", -123) == CASUAL_ORDER_INVALID_ARGUMENT);

   tpfree( buffer);
}


TEST( casual_order_buffer, DISABLED_performance__expecting_good_enough_speed)
{
   for( long idx = 0; idx < 100000; ++idx)
   {
      auto buffer = tpalloc( CASUAL_ORDER, "", 512);
      ASSERT_TRUE( buffer != nullptr);

      const long iterations = 10;

      for( long idx = 0; idx < iterations; ++idx)
      {
         //ASSERT_TRUE( casual_order_add_bool( &buffer, false) == CASUAL_ORDER_SUCCESS);
         ASSERT_TRUE( casual_order_add_char( &buffer, 'a') == CASUAL_ORDER_SUCCESS);
         ASSERT_TRUE( casual_order_add_short( &buffer, 123) == CASUAL_ORDER_SUCCESS);
         ASSERT_TRUE( casual_order_add_long( &buffer, 654321) == CASUAL_ORDER_SUCCESS);
         ASSERT_TRUE( casual_order_add_float( &buffer, 3.14) == CASUAL_ORDER_SUCCESS);
         ASSERT_TRUE( casual_order_add_double( &buffer, 987.654) == CASUAL_ORDER_SUCCESS);
         ASSERT_TRUE( casual_order_add_string( &buffer, "Hello!") == CASUAL_ORDER_SUCCESS);
         ASSERT_TRUE( casual_order_add_binary( &buffer, "Some Data", 9) == CASUAL_ORDER_SUCCESS);
      }

      for( long idx = 0; idx < iterations; ++idx)
      {

         //bool boolean;
         //ASSERT_TRUE( casual_order_get_bool( buffer, &boolean) == CASUAL_ORDER_SUCCESS);

         char character;
         ASSERT_TRUE( casual_order_get_char( buffer, &character) == CASUAL_ORDER_SUCCESS);

         short short_integer;
         ASSERT_TRUE( casual_order_get_short( buffer, &short_integer) == CASUAL_ORDER_SUCCESS);

         long long_integer;
         ASSERT_TRUE( casual_order_get_long( buffer, &long_integer) == CASUAL_ORDER_SUCCESS);

         float short_decimal;
         ASSERT_TRUE( casual_order_get_float( buffer, &short_decimal) == CASUAL_ORDER_SUCCESS);

         double long_decimal;
         ASSERT_TRUE( casual_order_get_double( buffer, &long_decimal) == CASUAL_ORDER_SUCCESS);


         const char* string = nullptr;
         ASSERT_TRUE( casual_order_get_string( buffer, &string) == CASUAL_ORDER_SUCCESS);

         const char* binary = nullptr;
         long size;
         ASSERT_TRUE( casual_order_get_binary( buffer, &binary, &size) == CASUAL_ORDER_SUCCESS) << casual_order_get_binary( buffer, &binary, &size);

      }

      tpfree( buffer);

   }
}

