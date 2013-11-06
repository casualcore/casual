//
// test_casual_order_buffer.cpp
//
//  Created on: 20 okt 2013
//      Author: Kristone
//




#include <gtest/gtest.h>

#include "common/order_buffer.h"


#include <string>



TEST( casual_order_buffer, allocate_with_enough_size__expecting_success)
{
   char buffer[512];
   EXPECT_TRUE( CasualOrderCreate( buffer, sizeof(buffer)) == CASUAL_ORDER_SUCCESS);
}

TEST( casual_order_buffer, reallocate_with_bigger_size__expecting_success)
{
   char buffer[64];
   ASSERT_TRUE( CasualOrderCreate( buffer, sizeof(buffer) / 2) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderExpand( buffer, sizeof(buffer)) == CASUAL_ORDER_SUCCESS);
}

TEST( casual_order_buffer, reallocate_with_smaller_size__expecting_success)
{
   char buffer[64];
   ASSERT_TRUE( CasualOrderCreate( buffer, sizeof(buffer)) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderReduce( buffer, 32) == CASUAL_ORDER_SUCCESS);
}

TEST( casual_order_buffer, reallocate_with_smaller_size__expecting_failure)
{
   char buffer[64];
   ASSERT_TRUE( CasualOrderCreate( buffer, sizeof(buffer)) == CASUAL_ORDER_SUCCESS);
   ASSERT_TRUE( CasualOrderAddString( buffer, "goin' casual today!") == CASUAL_ORDER_SUCCESS);

   long used = 0;
   ASSERT_TRUE( CasualOrderUsed( buffer, &used) == CASUAL_ORDER_SUCCESS);

   EXPECT_TRUE( CasualOrderReduce( buffer, used - 1) == CASUAL_ORDER_NO_SPACE);
}


TEST( casual_order_buffer, allocate_with_insufficient_size__expecting_failure)
{
   char buffer[5];
   EXPECT_TRUE( CasualOrderCreate( buffer, sizeof(buffer)) == CASUAL_ORDER_NO_SPACE);
}


TEST( casual_order_buffer, add_and_get)
{
   char buffer[512];
   ASSERT_TRUE( CasualOrderCreate( buffer, sizeof(buffer)) == CASUAL_ORDER_SUCCESS);

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


   const char* string;
   EXPECT_TRUE( CasualOrderGetString( buffer, &string) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( std::string( string) == "Hello!");

   const char* binary;
   long size;
   EXPECT_TRUE( CasualOrderGetBinary( buffer, &binary, &size) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( std::string( binary, size) == "Some Data");
   EXPECT_TRUE( size == 9);

}

TEST( casual_order_buffer, detect_out_of_space)
{
   char buffer[64];
   EXPECT_TRUE( CasualOrderCreate( buffer, sizeof(buffer)) == CASUAL_ORDER_SUCCESS);


   EXPECT_TRUE( CasualOrderAddLong( buffer, 123456) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderAddString( buffer, std::string( 'a', 64).c_str()) == CASUAL_ORDER_NO_SPACE);

}

TEST( casual_order_buffer, detect_out_of_place)
{
   char buffer[64];
   EXPECT_TRUE( CasualOrderCreate( buffer, sizeof(buffer)) == CASUAL_ORDER_SUCCESS);


   EXPECT_TRUE( CasualOrderAddLong( buffer, 123456) == CASUAL_ORDER_SUCCESS);
   long integer;
   EXPECT_TRUE( CasualOrderGetLong( buffer, &integer) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( CasualOrderGetLong( buffer, &integer) == CASUAL_ORDER_NO_PLACE);

}


TEST( casual_order_buffer, copy_buffer__expecting_success)
{
   char source[64];
   ASSERT_TRUE( CasualOrderCreate( source, sizeof(source)) == CASUAL_ORDER_SUCCESS);

   ASSERT_TRUE( CasualOrderAddChar( source, 'a') == CASUAL_ORDER_SUCCESS);
   ASSERT_TRUE( CasualOrderAddLong( source, 654321) == CASUAL_ORDER_SUCCESS);

   char target[64];
   ASSERT_TRUE( CasualOrderCreate( target, sizeof(target)) == CASUAL_ORDER_SUCCESS);


   EXPECT_TRUE( CasualOrderCopyBuffer( target, source) == CASUAL_ORDER_SUCCESS);


   char character;
   EXPECT_TRUE( CasualOrderGetChar( target, &character) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( character == 'a');

   long long_integer;
   EXPECT_TRUE( CasualOrderGetLong( target, &long_integer) == CASUAL_ORDER_SUCCESS);
   EXPECT_TRUE( long_integer == 654321);

}

