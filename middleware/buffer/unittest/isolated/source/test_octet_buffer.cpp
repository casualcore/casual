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

TEST( casual_octet_buffer, allocate_xml_with_normal_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_OCTET, CASUAL_OCTET_XML, 128);
   ASSERT_TRUE( buffer != nullptr);

   tpfree( buffer);
}

TEST( casual_octet_buffer, allocate_json_with_normal_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_OCTET, CASUAL_OCTET_JSON, 128);
   ASSERT_TRUE( buffer != nullptr);

   tpfree( buffer);
}

TEST( casual_octet_buffer, allocate_yaml_with_normal_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_OCTET, CASUAL_OCTET_YAML, 128);
   ASSERT_TRUE( buffer != nullptr);

   tpfree( buffer);
}

