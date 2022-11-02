//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/state/order.h"

#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace domain::manager::state::order
   {

      std::vector< state::dependency::Group> boot(
         const State& state,
         const std::vector< Server>& source_servers, 
         const std::vector< Executable>& source_executables)
      {
         Trace trace{ "domain::manager::state::create::boot::order"};

         auto group_wrapper = algorithm::container::vector::reference::create( state.groups);
         algorithm::stable_sort( group_wrapper, state::Group::boot::Order{});

         auto server_wrappers = algorithm::container::vector::reference::create( source_servers);
         auto excutable_wrappers = algorithm::container::vector::reference::create( source_executables);

         auto executables = range::make( excutable_wrappers);

         auto is_domain_manager = [&state]( const Server& server) { return server.id == state.manager_id;};
         auto servers = algorithm::remove_if( range::make( server_wrappers), is_domain_manager);

         auto batch_transform = [&]( const Group& group)
         {
            state::dependency::Group dependency;
            dependency.description = group.name;

            auto extract = [&]( auto& entities, auto& output)
            {
               // Partition executables so we get the ones that has current group as a dependency
               auto slice = algorithm::stable::partition( entities, [&]( const auto& e){
                  return static_cast< bool>( algorithm::find( e.get().memberships, group.id));
               });

               algorithm::transform( std::get< 0>( slice), output, []( auto& e){
                  common::traits::iterable::value_t< decltype( output)> result;
                  result = e.get().id;
                  return result;
               });

               return std::get< 1>( slice);
            };

            servers = extract( servers, dependency.servers);
            executables = extract( executables, dependency.executables);

            return dependency;
         };

         // Reverse the order, so we 'consume' executable based on the group
         // that is the farthest in the dependency chain
         auto result = algorithm::transform( algorithm::reverse( group_wrapper), batch_transform);

         // remove "empty"
         algorithm::container::trim( result, algorithm::remove_if( result, []( auto& group){ return group.servers.empty() && group.executables.empty();}));

         // We reverse the result so the dependency order is correct
         return algorithm::reverse( result);
      }


      std::vector< state::dependency::Group> boot( const State& state)
      {
         return boot( state, state.servers, state.executables);
      }

      std::vector< state::dependency::Group> shutdown( const State& state)
      {
         return algorithm::reverse( boot( state));
      }


   } // domain::manager::state::order
} // casual