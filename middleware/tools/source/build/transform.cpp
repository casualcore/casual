//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/build/transform.h"
#include "tools/common.h"

#include "common/algorithm/container.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

namespace casual
{
   namespace tools
   {
      namespace build
      {
         namespace transform
         {
            namespace local
            {
               namespace
               {
                  auto accumulate = []( auto& values, auto functor)
                  {
                     std::vector< std::string> result;

                     for( auto& value : values)
                        common::algorithm::append_unique( functor( value), result);

                     return result;
                  };
               } // <unnamed>
            } // local

            namespace paths
            {
               std::vector< std::string> include( const std::vector< model::Resource>& resources)
               {
                  return local::accumulate( resources, []( auto& r){ return r.paths.include;});
               }

               std::vector< std::string> library( const std::vector< model::Resource>& resources)
               {
                  return local::accumulate( resources, []( auto& r){ return r.paths.library;});
               }

            } // paths

            std::vector< std::string> libraries( const std::vector< model::Resource>& resources)
            {
               return local::accumulate( resources, []( auto& r){ return r.libraries;});
            }

            std::vector< model::Resource> resources( 
               const std::vector< configuration::build::model::Resource>& resources, 
               const std::vector< std::string>& keys,
               const configuration::model::system::Model& system)
            {
               Trace trace{ "tools::build::transform::resources"};

               auto transform_key = [&system]( auto& key)
               {
                  auto resource = common::algorithm::find( system.resources, key);

                  if( ! resource)
                     common::code::raise::error( common::code::casual::invalid_configuration, 
                        "failed to find resource for key: ", key, " - check resource configuration for casual");
                  
                  model::Resource result;

                  result.key = key;
                  result.libraries = resource->libraries;
                  result.xa_struct_name = resource->xa_struct_name;
                  result.paths.include = resource->paths.include;
                  result.paths.library = resource->paths.library;

                  return result;
               };

               auto transform_resource = [&transform_key]( auto& resource)
               {
                  auto result = transform_key( resource.key);
                  result.name = resource.name;
                  return result;
               };

               auto result = common::algorithm::transform( resources, transform_resource);

               common::algorithm::container::append( common::algorithm::transform( keys, transform_key), result);

               return result;
            }

            std::vector< model::Service> services( 
               const configuration::build::server::Model& model, 
               const std::vector< std::string>& names,
               const std::string& transaction_mode)
            {
               Trace trace{ "tools::build::transform::services"};

               auto result = common::algorithm::transform( model.server.services, []( auto& service)
               {
                  model::Service result{ service.name};
                  result.function = service.function.value_or( service.name);
                  result.category = service.category.value_or( "");
                  if( service.transaction)
                     result.transaction = common::service::transaction::mode( *service.transaction);
                  if( service.visibility)
                     result.visibility = common::service::visibility::transform( *service.visibility);
               
                  return result;
               });

               auto mode = transaction_mode.empty() ? common::service::transaction::Type::automatic : common::service::transaction::mode( transaction_mode);

               common::algorithm::transform( names, result, [mode]( auto& name)
               {
                  model::Service result{ name};
                  result.transaction = mode;
                  return result;
               });

               return result;
            }
         } // transform
      } // build
   } // tools
} // casual