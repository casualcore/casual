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

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);

         namespace predicate
         {
            namespace alias::has
            {
               auto instances( std::string_view expression, platform::size::type count)
               {
                  return [ expression = std::regex{ expression.data(), expression.size()}, count]( const manager::admin::model::State& state)
                  {  
                     // we do a copy of the range, since this is only used in unittests...
                     auto is_alias_has_count = [ &expression, count]( auto range)
                     {
                        auto matched = common::algorithm::filter( range, [ &expression]( auto& value){ return std::regex_match( value.alias, expression);});

                        return matched && common::algorithm::all_of( matched, [ count]( auto& value)
                        {
                           return common::algorithm::count_if( value.instances, []( auto& instance)
                           { 
                              return instance.state == decltype( instance.state)::running;}
                           ) == count;

                        });
                     };

                     return is_alias_has_count( state.servers) || is_alias_has_count( state.executables);
                  };
               }
            } // alias::has
            
         } // predicate
      }

   } // domain::unittest
} // casual