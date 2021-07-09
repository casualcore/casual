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

namespace casual
{
   using namespace common;

   namespace transaction::manager::transform
   {
      namespace local
      {
         namespace
         {
            auto initialize_log = []( auto&& configuration)
            {
               std::string file = environment::directory::domain() / "transaction" / "log.db";
               return common::environment::string( common::algorithm::coalesce(
                  std::move( configuration), 
                  std::move( file)));
            };


            auto resources = []( auto&& resources, const auto& properties)
            {
               Trace trace{ "transaction::manager::state::local::configure::resources"};

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
                  
                  common::event::error::send( code::casual::invalid_argument, event::error::Severity::fatal, "failed to correlate resource key: '", r.key, "'");
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

      State state( casual::configuration::model::transaction::Model model, std::vector< configuration::resource::Property> properties)
      {
         Trace trace{ "transaction::manager::transform::state"};

         State state;

         state.persistent.log = decltype( state.persistent.log){ local::initialize_log( model.log)};

         for( auto& property : properties)
         {
            auto [ pair, emplaced] = state.resource.properties.emplace( property.key, std::move( property));
            if( ! emplaced)
               event::error::raise( code::casual::invalid_configuration, event::error::Severity::fatal, "multiple keys in resource config: ", pair->first);
         }

         state.resources = local::resources( model.resources, state.resource.properties);
            
         for( auto& mapping : model.mappings)
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