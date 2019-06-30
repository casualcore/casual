//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/build/resource.h"
#include "tools/common.h"

#include "common/algorithm.h"
#include "common/string.h"
#include "common/exception/system.h"

namespace casual
{
   namespace tools
   {
      namespace build
      {

         namespace transform
         {
            std::vector< Resource> resources( 
               const std::vector< configuration::build::Resource>& resources, 
               const std::vector< configuration::resource::Property>& properties)
            {
               Trace trace{ "tools::build::transform::resources"};

               return common::algorithm::transform( resources, [&properties]( auto& resource)
               {
                  auto property = common::algorithm::find_if( properties, [&resource]( auto& property){ return property.key == resource.key;});

                  if( ! property)
                  {
                     throw common::exception::system::invalid::Argument{
                        common::string::compose( "failed to find resource property for key: ", resource.key, 
                        " - check resource configuration for casual")
                     };
                  }

                  Resource result;

                  result.key = resource.key;
                  result.name = resource.name;
                  result.libraries = property->libraries;
                  result.xa_struct_name = property->xa_struct_name;
                  result.paths.include = property->paths.include;
                  result.paths.library = property->paths.library;

                  return result;
               });
            }
         } // resource
      } // build
   } // tools
} // casual