//
// test_casual_order_buffer.cpp
//
//  Created on: 20 okt 2013
//      Author: Kristone
//




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
   EXPECT_TRUE( CasualOrderExploreBuffer( buffer, nullptr, &used) == CASUAL_ORDER_SUCCESS);
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

   EXPECT_TRUE( CasualOrderAddBool( buffer, false) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderAddChar( buffer, 'a') == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderAddShort( buffer, 123) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderAddLong( buffer, 654321) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderAddFloat( buffer, 3.14) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderAddDouble( buffer, 987.654) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderAddString( buffer, "Hello!") == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderAddBinary( buffer, "Some Data", 9) == CASUAL_ORDER_SUCCESS);


   bool boolean;
   EXPECT_TRUE( CasualOrderGetBool( buffer, &boolean) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( boolean == false);


   char character;
   EXPECT_TRUE( CasualOrderGetChar( buffer, &character) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( character == 'a');

   short short_integer;
   EXPECT_TRUE( CasualOrderGetShort( buffer, &short_integer) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( short_integer == 123);

   long long_integer;
   EXPECT_TRUE( CasualOrderGetLong( buffer, &long_integer) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( long_integer == 654321);

   float short_decimal;
   EXPECT_TRUE( CasualOrderGetFloat( buffer, &short_decimal) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( short_decimal > 3.1 && short_decimal < 3.2);

   double long_decimal;
   EXPECT_TRUE( CasualOrderGetDouble( buffer, &long_decimal) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( long_decimal > 987.6 && long_decimal < 987.7);


   const char* string = nullptr;
   EXPECT_TRUE( CasualOrderGetString( buffer, &string) == CASUAL_ORDER_SUCCESS);
   EXPECT_STREQ( string, "Hello!") << string;

   const char* binary = nullptr;
   long size;
   EXPECT_TRUE( CasualOrderGetBinary( buffer, &binary, &size) == CASUAL_ORDER_SUCCESS) << CasualOrderGetBinary( buffer, &binary, &size);
   EXPECT_TRUE( std::string( binary, size) == "Some Data") << std::string( binary, size) << binary << " " << size << std::endl;
   EXPECT_TRUE( size == 9);


   tpfree( buffer);
}

TEST( casual_order_buffer, detect_out_of_space)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 64);
   ASSERT_TRUE( buffer != nullptr);


   EXPECT_TRUE( CasualOrderAddLong( buffer, 123456) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderAddString( buffer, std::string( 'a', 64).c_str()) == CASUAL_ORDER_NO_SPACE);

   tpfree( buffer);
}

TEST( casual_order_buffer, detect_out_of_place)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 64);
   ASSERT_TRUE( buffer != nullptr);


   EXPECT_TRUE( CasualOrderAddLong( buffer, 123456) == CASUAL_ORDER_SUCCESS);
   long integer;
   EXPECT_TRUE( CasualOrderGetLong( buffer, &integer) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderGetLong( buffer, &integer) == CASUAL_ORDER_NO_PLACE);

   tpfree( buffer);
}

TEST( casual_order_buffer, add_binary_data_with_negative_size__expecting_invalid_argument)
{
   auto buffer = tpalloc( CASUAL_ORDER, "", 0);
   ASSERT_TRUE( buffer != nullptr);

   EXPECT_TRUE( CasualOrderAddBinary( buffer, "some data", -123) == CASUAL_ORDER_INVALID_ARGUMENT);

   tpfree( buffer);
}



TEST( casual_order_buffer, copy_buffer__expecting_success)
{
   auto source = tpalloc( CASUAL_ORDER, "", 64);
   ASSERT_TRUE( source != nullptr);

   ASSERT_TRUE( CasualOrderAddChar( source, 'a') == CASUAL_ORDER_SUCCESS);
   ASSERT_TRUE( CasualOrderAddLong( source, 654321) == CASUAL_ORDER_SUCCESS);

   auto target = tpalloc( CASUAL_ORDER, "", 64);
   ASSERT_TRUE( target != nullptr);

   EXPECT_TRUE( CasualOrderCopyBuffer( target, source) == CASUAL_ORDER_SUCCESS);


   char character;
   EXPECT_TRUE( CasualOrderGetChar( target, &character) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( character == 'a');

   long long_integer;
   EXPECT_TRUE( CasualOrderGetLong( target, &long_integer) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( long_integer == 654321);

   tpfree( source);
   tpfree( target);
}

