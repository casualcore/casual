//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "casual/xatmi/extended.h"

#include "common/execution.h"

#include "uuid/uuid.h"

namespace casual
{
   namespace common
   {
      TEST( xatmi_execution, execution_id_get_size_is_32)
      {
         common::unittest::Trace trace;

         auto id = casual_execution_id_get();

         EXPECT_EQ( uuid::string( *id).size(), 32U);
      }

      TEST( xatmi_execution, execution_id_set)
      {
         common::unittest::Trace trace;

         uuid_t uuid;
         uuid_generate( uuid);

         casual_execution_id_set( &uuid);

         auto get_id = casual_execution_id_get();

         EXPECT_EQ( uuid::string( uuid), uuid::string( *get_id));
      }
   } // common
} // casual

