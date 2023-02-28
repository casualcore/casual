//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/unittest/utility.h"

#include "domain/manager/admin/server.h"

#include "serviceframework/service/protocol/call.h"

#include "common/unittest.h"

namespace casual
{
   using namespace common;
   namespace domain::unittest
   {
      namespace local
      {
         namespace
         {
            auto find_instance = []( auto& range, auto& alias, auto index)
               -> decltype( range.at( 0).instances.at( index).handle)
            {
               if( auto found = algorithm::find( range, alias))
                  return found->instances.at( index).handle;

               return {};
            };
         } // <unnamed>
      } // local

      manager::admin::model::State state()
      {
         common::unittest::service::wait::until::advertised( manager::admin::service::name::state);
         serviceframework::service::protocol::binary::Call call;
         auto reply = call( manager::admin::service::name::state);
         return reply.extract< manager::admin::model::State>();
      }

      common::process::Handle server( const manager::admin::model::State& state, std::string_view alias, platform::size::type index)
      {
         return local::find_instance( state.servers, alias, index);
      }

      common::strong::process::id executable( const manager::admin::model::State& state, std::string_view alias, platform::size::type index)
      {
         return local::find_instance( state.executables, alias, index);
      }

      namespace fetch::predicate
      {
         namespace alias::has
         {
            auto instances( std::string_view expression, platform::size::type count) -> common::unique_function< bool( const manager::admin::model::State&)>
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
            
      } // fetch::predicate

   } // domain::unittest
} // casual
