//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/admin/transform.h"

#include "domain/discovery/common.h"

#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace domain::discovery::admin
   {
      namespace local
      {
         namespace
         {
            namespace transform
            {
               auto providers( const state::Providers& providers)
               {
                  return algorithm::transform( providers.all(), []( auto& provider)
                  {
                     model::Provider result;
                     result.abilities = provider.abilities;
                     result.process = provider.process;
                     return result;
                  });
               }

               auto pending( const State& state)
               {
                  model::Pending result;

                  result.send = algorithm::transform( state.multiplex.destinations(), []( auto& destination)
                  {
                     model::pending::Send result;
                     result.destination.ipc = destination.destination().ipc();
                     result.destination.socket = destination.destination().socket().descriptor();

                     result.messages = algorithm::accumulate( destination.queue(), decltype( result.messages){}, []( auto result, auto& message)
                     {
                        if( auto found = algorithm::find( result, message.type()))
                           ++found->count;
                        else
                           result.push_back( model::pending::Message{ message.type(), 1});

                        return result;
                     });


                     return result;
                  });
                  
                  return result;
               }
            } // transform
         } // <unnamed>
      } // local

      model::State transform( const State& state)
      {
         Trace trace{ "domain::discovery::admin::transform"};

         model::State result;

         result.runlevel = state.runlevel();
         result.providers = local::transform::providers( state.providers);
         result.pending = local::transform::pending( state);

         return result;
      }

   } // domain::discovery::admin
} // casual
