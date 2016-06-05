//
// test_casual_order_buffer.cpp
//
//  Created on: 3 nov 2013
//      Author: Kristone
//


#include <gtest/gtest.h>

#include "buffer/octet.h"
#include "xatmi.h"


TEST( casual_octet_buffer, allocate_with_normal_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_OCTET, "", 128);
   ASSERT_TRUE( buffer != nullptr);

   tpfree( buffer);
}

TEST( casual_octet_buffer, allocate_json_with_normal_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_OCTET, CASUAL_OCTET_JSON, 64);
   ASSERT_TRUE( buffer != nullptr);

   const char* name{};
   long size{};

   EXPECT_FALSE( casual_octet_explore_buffer( buffer, &name, &size));

   EXPECT_STREQ( name, CASUAL_OCTET_JSON);
   EXPECT_TRUE( size == 64);


   tpfree( buffer);
}

TEST( casual_octet_buffer, allocate_xml_with_normal_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_OCTET, CASUAL_OCTET_XML, 32);
   ASSERT_TRUE( buffer != nullptr);

   const char* name{};
   long size{};

   EXPECT_FALSE( casual_octet_explore_buffer( buffer, &name, &size));

   EXPECT_STREQ( name, CASUAL_OCTET_XML);
   EXPECT_TRUE( size == 32);


   tpfree( buffer);
}


TEST( casual_octet_buffer, allocate_yaml_with_normal_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_OCTET, CASUAL_OCTET_YAML, 16);
   ASSERT_TRUE( buffer != nullptr);

   const char* name{};
   long size{};

   EXPECT_FALSE( casual_octet_explore_buffer( buffer, &name, &size));

   EXPECT_STREQ( name, CASUAL_OCTET_YAML);
   EXPECT_TRUE( size == 16);


   tpfree( buffer);
}

TEST( casual_octet_buffer, write_to_buffer__expecting_automatic_resize)
{
   auto buffer = tpalloc( CASUAL_OCTET, nullptr, 0);
   ASSERT_TRUE( buffer != nullptr);

   const char data[] = "some casual data";

   EXPECT_FALSE( casual_octet_set( &buffer, data, sizeof( data)));

   EXPECT_TRUE( tptypes( buffer, nullptr, nullptr) == sizeof( data));

   tpfree( buffer);
}


