//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/build/model.h"

#include "configuration/resource/property.h"
#include "configuration/build/resource.h"
#include "configuration/build/server.h"

namespace casual
{
   namespace tools
   {
      namespace build
      {
         namespace transform
         {
            namespace paths
            {
               std::vector< std::string> include( const std::vector< model::Resource>& resources);
               std::vector< std::string> library( const std::vector< model::Resource>& resources);

            } // paths

            std::vector< std::string> libraries( const std::vector< model::Resource>& resources);


            std::vector< model::Resource> resources( 
               const std::vector< configuration::build::Resource>& resources, 
               const std::vector< std::string>& keys,
               const std::vector< configuration::resource::Property>& properties);


            std::vector< model::Service> services( 
               const std::vector< configuration::build::server::Service>& services, 
               const std::vector< std::string>& names);
         } // transform
      } // build
   } // tools
} // casual