//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/environment/variable.h"

namespace casual
{
   namespace common
   {
      namespace environment
      {
         TEST( common_environment_variable, default_ctor)
         {
            common::unittest::Trace trace;

            Variable variable;
            EXPECT_TRUE( variable.empty());
            EXPECT_TRUE( variable.name().empty());
            EXPECT_TRUE( variable.value().empty());
         }

         TEST( common_environment_variable, name_value)
         {
            common::unittest::Trace trace;

            Variable variable{ "NAME=VALUE"};
            EXPECT_TRUE( ! variable.empty());
            EXPECT_TRUE( variable.name() == "NAME");
            EXPECT_TRUE( variable.value() == "VALUE");
         }

         TEST( common_environment_variable, string_name_value)
         {
            common::unittest::Trace trace;

            Variable variable{ "NAME=value"};
            EXPECT_TRUE( ! variable.empty());
            EXPECT_TRUE( variable.name() == std::string{ "NAME"});
            EXPECT_TRUE( variable.value() == std::string{ "value"});
         }


         TEST( common_environment_variable, name_empty_value)
         {
            common::unittest::Trace trace;

            Variable variable{ "NAME="};
            EXPECT_TRUE( ! variable.empty());
            EXPECT_TRUE( variable.name() == "NAME");
            EXPECT_TRUE( variable.value().empty());
         }

         TEST( common_environment_variable, copy_ctor)
         {
            common::unittest::Trace trace;
           
            Variable origin{ "NAME=VALUE"};
            auto variable = origin;
            EXPECT_TRUE( ! origin.empty());
            EXPECT_TRUE( variable == origin);
         }

         TEST( common_environment_variable, move_ctor)
         {
            common::unittest::Trace trace;
           
            Variable moved_from{ "NAME=VALUE"};
            auto variable = std::move( moved_from);
            EXPECT_TRUE( moved_from.empty());
            EXPECT_TRUE( ! variable.empty());
            EXPECT_TRUE( variable.name() == "NAME");
            EXPECT_TRUE( variable.value() == "VALUE");
         }

         TEST( common_environment_variable, move_assign)
         {
            common::unittest::Trace trace;
           
            Variable moved_from{ "NAME=VALUE"};
            
            Variable variable;
            variable = std::move( moved_from);

            EXPECT_TRUE( moved_from.empty());
            EXPECT_TRUE( ! variable.empty());
            EXPECT_TRUE( variable.name() == "NAME");
            EXPECT_TRUE( variable.value() == "VALUE");
         }

         TEST( common_environment_variable, string_assign)
         {
            common::unittest::Trace trace;
           
            std::string string{ "NAME=VALUE"};
            Variable variable{ "some-other-name=foo"};
            variable = string;

            EXPECT_TRUE( ! variable.empty());
            EXPECT_TRUE( variable.name() == "NAME");
            EXPECT_TRUE( variable.value() == "VALUE");
         }

         TEST( common_environment_variable, string_move_assign)
         {
            common::unittest::Trace trace;
           
            std::string string{ "NAME=VALUE"};
            Variable variable{ "some-other-name=foo"};
            variable = std::move( string);

            EXPECT_TRUE( ! variable.empty());
            EXPECT_TRUE( variable.name() == "NAME");
            EXPECT_TRUE( variable.value() == "VALUE");
         }




      } // environment
   } // common
} // casual