//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/manager/transform.h"
#include "transaction/common.h"

#include "common/event/send.h"
#include "common/environment.h"
#include "common/environment/normalize.h"
#include "common/algorithm/coalesce.h"

#include "configuration/system.h"

namespace casual
{
   using namespace common;

   namespace transaction::manager::transform
   {
      namespace local
      {
         namespace
         {
            template< typename T>
            std::filesystem::path initialize_log( T configuration)
            {
               Trace trace{ "transaction::manager::transfrom::local::initialize_log"};

               if( ! configuration.empty())
                  return common::environment::expand( std::move( configuration));

               auto file = environment::directory::transaction() / "log.db";

               // TODO: remove this in 2.0 (that exist to be backward compatible)
               {
                  // if the wanted path exists, we can't overwrite with the old
                  if( std::filesystem::exists( file))
                     return file;

                  auto old = environment::directory::domain() / "transaction" / "log.db";
                  if( std::filesystem::exists( old))
                  {
                     std::filesystem::rename( old, file);
                     event::notification::send( "transaction log file moved: ", std::filesystem::relative( old), " -> ", std::filesystem::relative( file));
                     log::line( log::category::warning, "transaction log file moved: ", old, " -> ", file);
                  }
               }

               return file;
            };


            auto resources = []( auto&& resources, const auto& properties)
            {
               Trace trace{ "transaction::manager::transform::local::resources"};

               auto transform_resource = []( const auto& configuration)
               {
                  state::resource::Proxy result{ configuration};

                  // make sure we've got a name
                  result.configuration.name = common::algorithm::coalesce( 
                     std::move( result.configuration.name), 
                     common::string::compose( ".rm.", result.configuration.key, '.', result.id));

                  return result;
               };

               auto validate = [&]( const auto& r) 
               {
                  if( common::algorithm::find( properties, r.key))
                     return true;
                  
                  common::event::error::fatal::send( code::casual::invalid_argument, "failed to correlate resource key: '", r.key, "'");
                  return false;   
               };

               std::vector< state::resource::Proxy> result;

               common::algorithm::transform_if(
                  resources,
                  result,
                  transform_resource,
                  validate);

               return result;
            };

         } // <unnamed>
      } // local

      State state( casual::configuration::Model model)
      {
         Trace trace{ "transaction::manager::transform::state"};

         State state;

         state.persistent.log = decltype( state.persistent.log){ local::initialize_log( std::move( model.transaction.log))};

         auto get_system = []( auto system)
         {
            if( system != decltype( system){})
               return system;

            // TODO deprecated, will be removed at some point
            return configuration::system::get();
         };

         state.system.configuration =  get_system( std::move( model.system));

         state.resources = local::resources( model.transaction.resources, state.system.configuration.resources);
            
         for( auto& mapping : model.transaction.mappings)
         {
            state.alias.configuration.emplace( mapping.alias, algorithm::transform( mapping.resources, [&state]( auto& name)
            {
               return state.get_resource( name).id;
            }));
         }

         return state;
      }
      
   } // transaction::manager::transform
} // casual