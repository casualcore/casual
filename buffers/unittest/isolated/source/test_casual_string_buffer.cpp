//
// test_casual_string_buffer.cpp
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#include <gtest/gtest.h>

#include "casual_string_buffer.h"

#include <cstring>

extern long CasualStringCreate( char* buffer, long size);
extern long CasualStringExpand( char* buffer, long size);
extern long CasualStringReduce( char* buffer, long size);
extern long CasualStringNeeded( char* buffer, long size);


TEST( casual_string_buffer, allocate_with_normal_size__expecting_success)
{
   char buffer[64];
   EXPECT_TRUE( CasualStringCreate( buffer, sizeof(buffer)) == CASUAL_STRING_SUCCESS);
}

TEST( casual_string_buffer, allocate_with_normal_size__expecting_size_zero)
{
   char buffer[64];
   EXPECT_TRUE( CasualStringCreate( buffer, sizeof(buffer)) == CASUAL_STRING_SUCCESS);
   EXPECT_TRUE( std::strlen(buffer) == 0);
}

TEST( casual_string_buffer, allocate_with_zero_size__expecting_failure)
{
   char buffer[0];
   EXPECT_TRUE( CasualStringCreate( buffer, sizeof(buffer)) == CASUAL_STRING_NO_SPACE);
}

TEST( casual_string_buffer, reallocate_with_less_size__expecting_success)
{
   char buffer[64];
   EXPECT_TRUE( CasualStringCreate( buffer, sizeof(buffer)) == CASUAL_STRING_SUCCESS);
   EXPECT_TRUE( CasualStringReduce( buffer, sizeof(buffer) / 2) == CASUAL_STRING_SUCCESS);
}

TEST( casual_string_buffer, reallocate_with_greater_size__expecting_success)
{
   char buffer[64];
   EXPECT_TRUE( CasualStringCreate( buffer, sizeof(buffer) / 2) == CASUAL_STRING_SUCCESS);
   EXPECT_TRUE( CasualStringExpand( buffer, sizeof(buffer)) == CASUAL_STRING_SUCCESS);
}

TEST( casual_string_buffer, allocate_and_write_and_reallocate_with_size_too_small__expecting_failure)
{
   char buffer[32];
   EXPECT_TRUE( CasualStringCreate( buffer, sizeof(buffer)) == CASUAL_STRING_SUCCESS);
   std::strcpy( buffer, "let's go bananas someday");
   EXPECT_TRUE( CasualStringReduce( buffer, sizeof(buffer) / 2) == CASUAL_STRING_NO_SPACE);
}
