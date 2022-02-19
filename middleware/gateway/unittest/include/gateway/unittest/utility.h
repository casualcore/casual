//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "gateway/manager/admin/model.h"

#include <vector>
#include <string>

namespace casual
{
   namespace gateway::unittest
   {
      using namespace common::unittest;


      manager::admin::model::State state();

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);

         namespace predicate
         {
            namespace is
            {
               inline auto outbound()
               { 
                  return []( auto& connection)
                  {
                     return connection.bound == decltype( connection.bound)::out;
                  };
               }

               namespace connected
               {
                  inline auto outbound()
                  { 
                     return []( auto& connection)
                     {
                        return ( ! is::outbound()( connection)) || connection.remote.id;
                     };
                  }
               } // connected

            } // is
            namespace outbound
            {
               inline auto connected()
               {
                  return []( auto& state)
                  {
                     return common::algorithm::all_of( state.connections, is::connected::outbound());
                  };
               }

               inline auto connected( platform::size::type count)
               {
                  return [count]( auto& state)
                  {
                     return common::algorithm::count_if( state.connections, is::connected::outbound()) == count;
                  };
               }

               inline auto connected( std::vector< std::string_view> names)
               {
                  return [ names = std::move( names)]( auto& state)
                  {
                     return common::algorithm::includes( state.connections, names);
                  };
               }

               inline auto connected( std::string_view name)
               {
                  return connected( std::vector< std::string_view>{ name});
               }

               // returns a predicate that checks if all out-connections has NOT a 'remote id'
               inline auto disconnected()
               {
                  return []( auto& state)
                  {
                     return common::algorithm::all_of( state.connections, []( auto& connection)
                     {
                        return connection.bound != decltype( connection.bound)::out || ! connection.remote.id;
                     }); 
                  };
               };

               inline auto routing( std::vector< std::string_view> services, std::vector< std::string_view> queues)
               {
                  return [services = std::move( services), queues = std::move( queues)]( auto& state)
                  {
                     return common::algorithm::includes( state.services, services)
                        && common::algorithm::includes( state.queues, queues);
                  };
               }
            } // outbound

            inline auto listeners( platform::size::type count = 1)
            {
               return [ count]( auto& state)
               {
                  return common::algorithm::count_if( state.listeners, []( auto& listener)
                  {
                     return listener.created > platform::time::point::type{};
                  }) == count;

               };
            }
               
         } // predicate
      } // fetch
      
   } // gateway::unittest
} // casual