//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/build/transform.h"
#include "tools/common.h"

#include "common/algorithm.h"
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
               const std::vector< configuration::build::Resource>& resources,
               const std::vector< std::string>& keys,
               const std::vector< configuration::resource::Property>& properties)
            {
               Trace trace{ "tools::build::transform::resources"};

               auto transform_key = [&properties]( auto& key)
               {
                  auto property = common::algorithm::find_if( properties, [&key]( auto& property){ return property.key == key;});

                  if( ! property)
                     common::code::raise::error( common::code::casual::invalid_configuration, 
                        "failed to find resource property for key: ", key, " - check resource configuration for casual");
                  
                  model::Resource result;

                  result.key = key;
                  result.libraries = property->libraries;
                  result.xa_struct_name = property->xa_struct_name;
                  result.paths.include = property->paths.include;
                  result.paths.library = property->paths.library;

                  return result;
               };

               auto transform_resource = [&transform_key]( auto& resource)
               {
                  auto result = transform_key( resource.key);
                  result.name = resource.name;
                  return result;
               };

               auto result = common::algorithm::transform( resources, transform_resource);

               common::algorithm::append( common::algorithm::transform( keys, transform_key), result);

               return result;
            }

            std::vector< model::Service> services( 
               const std::vector< configuration::build::server::Service>& services, 
               const std::vector< std::string>& names,
               const std::string& transaction_mode)
            {
               Trace trace{ "tools::build::transform::services"};

               auto result = common::algorithm::transform( services, []( auto& service)
               {
                  model::Service result{ service.name};
                  result.function = service.function.value_or( service.name);
                  result.category = service.category.value_or( "");
                  if( service.transaction)
                     result.transaction = common::service::transaction::mode( service.transaction.value());

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