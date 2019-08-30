//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/state/create.h"

#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {
         namespace state
         {
            namespace create
            {
               namespace boot 
               {
                  std::vector< state::Batch> order( 
                     const std::vector< Server>& source_servers, 
                     const std::vector< Executable>& source_executables,
                     const std::vector< Group>& source_groups)
                  {
                     Trace trace{ "domain::manager::state::create::boot::order"};

                     auto group_wrapper = range::to_reference( source_groups);
                     algorithm::stable_sort( group_wrapper, state::Group::boot::Order{});

                     auto server_wrappers = range::to_reference( source_servers);
                     auto excutable_wrappers = range::to_reference( source_executables);

                     auto executables = range::make( excutable_wrappers);
                     auto servers = range::make( server_wrappers);

                     auto batch_transform = [&]( const Group& group)
                     {
                        state::Batch batch{ group.id};

                        auto extract = [&]( auto& entities, auto& output)
                        {
                           // Partition executables so we get the ones that has current group as a dependency
                           auto slice = algorithm::stable_partition( entities, [&]( const auto& e){
                              return static_cast< bool>( algorithm::find( e.get().memberships, group.id));
                           });

                           algorithm::transform( std::get< 0>( slice), output, []( auto& e){
                              common::traits::iterable::value_t< decltype( output)> result;
                              result = e.get().id;
                              return result;
                           });

                           return std::get< 1>( slice);
                        };

                        servers = extract( servers, batch.servers);
                        executables = extract( executables, batch.executables);

                        return batch;
                     };

                     // Reverse the order, so we 'consume' executable based on the group
                     // that is the farthest in the dependency chain
                     auto result = algorithm::transform( algorithm::reverse( group_wrapper), batch_transform);

                     // remove "empty"
                     algorithm::trim( result, algorithm::remove_if( result, []( auto& batch){ return batch.empty();}));

                     // We reverse the result so the dependency order is correct
                     return algorithm::reverse( result);
                  }
               } // dependency   
               
            } // create
         } // state
      } // manager
   } // domain
} // casual