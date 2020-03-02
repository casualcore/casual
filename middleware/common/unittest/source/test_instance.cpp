//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/instance.h"
#include "common/string.h"

namespace casual
{
   namespace common
   {

      TEST( common_instance, empty)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ! instance::information().has_value());
      }


      TEST( common_instance, create_variable)
      {
         common::unittest::Trace trace;

         instance::Information information{ "alias", 42};

         auto variable = instance::variable( information);

         auto found = algorithm::find( variable, '=');
         
         EXPECT_TRUE( found.size() > 7 ) << "variable: " << variable;
      }
   }

} // casual