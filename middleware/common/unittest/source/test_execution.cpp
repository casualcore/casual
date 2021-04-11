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

         EXPECT_EQ( uuid::string( id.value()).size(), 32U);
      }

      TEST( casual_common_execution, c_execution_set)
      {
         common::unittest::Trace trace;

         auto set_id = execution::type{ uuid::make()};

         execution::id( set_id);

         auto get_id = execution::id();

         EXPECT_EQ( set_id, get_id);
      }


   } // common
} // casual
