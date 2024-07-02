//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>

#include "common/unittest.h"
#include "common/execution/context.h"
#include "common/uuid.h"

namespace casual
{

   namespace common
   {
      TEST( common_execution_context, execution_id)
      {
         common::unittest::Trace trace;

         auto id = execution::context::get().id;

         EXPECT_EQ( uuid::string( id.value()).size(), 32U);
      }

      TEST( common_execution_context, execution_set)
      {
         common::unittest::Trace trace;

         auto set_id = execution::type::generate();

         execution::context::id::set( set_id);

         auto get_id = execution::context::get().id;

         EXPECT_EQ( set_id, get_id);
      }

      TEST( common_execution_context, span)
      {
         common::unittest::Trace trace;

         auto span = strong::execution::span::id::generate();

         execution::context::span::set( span);

         EXPECT_TRUE( span == execution::context::get().span) << CASUAL_NAMED_VALUE( span);
      }


   } // common
} // casual
