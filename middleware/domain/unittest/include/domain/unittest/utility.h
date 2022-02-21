//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "domain/manager/admin/model.h"

namespace casual
{
   namespace domain::unittest
   {
      manager::admin::model::State state();

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);

         namespace predicate
         {
            namespace alias::has
            {
               auto instances( std::string_view alias, platform::size::type count)
               {
                  return [ alias, count]( const manager::admin::model::State& state)
                  {
                     auto is_alias_has_count = [ alias, count]( auto& range)
                     {
                        if( auto found = common::algorithm::find( range, alias))
                           return common::algorithm::count_if( found->instances, []( auto& instance)
                           {
                              return instance.state == decltype( instance.state)::running;
                           }) == count;

                        return false;
                     };

                     return is_alias_has_count( state.servers) || is_alias_has_count( state.executables);

                  };
               }
            } // alias::has
            
         } // predicate
      }

   } // domain::unittest
} // casual