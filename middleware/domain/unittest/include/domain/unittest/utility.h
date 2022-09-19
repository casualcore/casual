//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "domain/manager/admin/model.h"

#include <regex>

namespace casual
{
   namespace domain::unittest
   {
      manager::admin::model::State state();

      common::process::Handle server( const manager::admin::model::State& state, std::string_view alias, platform::size::type index = 0);
      common::strong::process::id executable( const manager::admin::model::State& state, std::string_view alias, platform::size::type index = 0);

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);

         namespace predicate
         {
            namespace alias::has
            {
               auto instances( std::string_view expression, platform::size::type count) -> common::unique_function< bool( const manager::admin::model::State&)>;

            } // alias::has
            
         } // predicate
      } // fetch

   } // domain::unittest
} // casual