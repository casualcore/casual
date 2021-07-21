//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/environment/scoped.h"

namespace casual
{
   namespace common
   {   
      TEST( common_environment_scoped, set_scoped__expect_env_variable_during_scope__expect_absent_after)
      {
         common::unittest::Trace trace;

         constexpr std::string_view name = "e7854e76fe6141f59c57a5cf52e49fba";


         ASSERT_TRUE( ! environment::variable::exists( name));

         {
            auto scoped = environment::variable::scoped::set( name, "foo");
            ASSERT_TRUE( environment::variable::exists( name));
            EXPECT_TRUE( environment::variable::get( name) == "foo");
         }

         ASSERT_TRUE( ! environment::variable::exists( name));
         
      }

      TEST( common_environment_scoped, move_scope__expect_varable_to_exist_during_scope)
      {
         common::unittest::Trace trace;

         constexpr std::string_view name = "e7854e76fe6141f59c57a5cf52e49fba";


         ASSERT_TRUE( ! environment::variable::exists( name));

         {
            auto scoped = environment::variable::scoped::set( name, "foo");
            auto moved = std::move( scoped);
            ASSERT_TRUE( environment::variable::exists( name));
            EXPECT_TRUE( environment::variable::get( name) == "foo");
         }

         ASSERT_TRUE( ! environment::variable::exists( name));
         
      }

      TEST( common_environment_scoped, set_origin__set_scoped__expect_env_variable_during_scope__expect_origin_after)
      {
         common::unittest::Trace trace;

         constexpr std::string_view name = "e7854e76fe6141f59c57a5cf52e49fba";

         environment::variable::set( name, "bar");
         ASSERT_TRUE( environment::variable::exists( name));
         EXPECT_TRUE( environment::variable::get( name) == "bar");


         {
            auto scoped = environment::variable::scoped::set( name, "foo");
            ASSERT_TRUE( environment::variable::exists( name));
            EXPECT_TRUE( environment::variable::get( name) == "foo");
         }

         ASSERT_TRUE( environment::variable::exists( name));
         EXPECT_TRUE( environment::variable::get( name) == "bar");
         
      }

   } // common
} // casual