//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>

#include "common/unittest.h"
#include "common/execution.h"
#include "common/uuid.h"

namespace casual
{

   namespace common
   {
      TEST( casual_common_execution, execution_id)
      {
         common::unittest::Trace trace;

         auto id = execution::id();

         EXPECT_EQ( uuid::string(id).size(), 32U);
      }

      TEST( casual_common_execution, c_execution_set)
      {
         common::unittest::Trace trace;

         Uuid set_uuid;

         execution::id( set_uuid);

         auto get_uuid = execution::id();

         EXPECT_EQ( uuid::string(set_uuid), uuid::string(get_uuid));
      }


   } // common
} // casual
