//
// test_casual_string_buffer.cpp
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#include <gtest/gtest.h>

#include "buffer/string.h"
#include "xatmi.h"


TEST( casual_string_buffer, allocate_with_normal_size__expecting_success)
{
   auto buffer = tpalloc( CASUAL_STRING, "", 32);
   ASSERT_TRUE( buffer != nullptr);

   tpfree( buffer);
}


