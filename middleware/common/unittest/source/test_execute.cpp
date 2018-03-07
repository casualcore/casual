//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>

#include "common/unittest.h"
#include "common/execute.h"


namespace casual
{

   namespace common
   {
      TEST( casual_common_execute, execute_once)
      {
         common::unittest::Trace trace;

         long executions = 0;

         for( int count = 0; count < 10; ++count)
         {
            execute::once( [&](){
               ++executions;
            });
         }

         EXPECT_TRUE( executions == 1);
      }


   } // common
} // casual
