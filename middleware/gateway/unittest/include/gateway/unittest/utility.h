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

               inline auto inbound()
               { 
                  return []( auto& connection)
                  {
                     return connection.bound == decltype( connection.bound)::in;
                  };
               }

               namespace runlevel
               {
                  inline auto connecting()
                  { 
                     return []( auto& connection)
                     {
                        return connection.runlevel == decltype( connection.runlevel)::connecting;
                     };
                  }

                  inline auto connected()
                  { 
                     return []( auto& connection)
                     {
                        return connection.runlevel == decltype( connection.runlevel)::connected && connection.remote.id;
                     };
                  }

                  inline auto failed()
                  { 
                     return []( auto& connection)
                     {
                        return connection.runlevel == decltype( connection.runlevel)::failed;
                     };
                  } 
               } // runlevel

               namespace connected
               {
                  inline auto outbound()
                  { 
                     return common::predicate::conjunction( is::outbound(), is::runlevel::connected());
                  }

                  inline auto inbound()
                  { 
                     return common::predicate::conjunction( is::inbound(), is::runlevel::connected());
                  }
               } // connected


            } // is
            namespace outbound
            {
               inline auto connected()
               {
                  return []( auto& state)
                  {
                     auto outbounds = common::algorithm::filter( state.connections, is::outbound());
                     return common::algorithm::all_of( outbounds, is::runlevel::connected());
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

               //! keep trying until the sum of all groups pending tasks is >= count
               inline auto pending( platform::size::type count)
               {
                  return [ count]( auto& state)
                  {
                     return common::algorithm::accumulate( state.outbound.groups, platform::size::type{}, []( auto result, auto& group)
                     {
                        return result + group.pending.tasks.size();
                     }) >= count;
                  };
               }
            } // outbound

            namespace inbound
            {
               inline auto connected( platform::size::type count)
               {
                  return [count]( auto& state)
                  {
                     return common::algorithm::count_if( state.connections, is::connected::inbound()) == count;
                  };
               }
               
            } // inbound

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