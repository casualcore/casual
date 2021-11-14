//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/build/model.h"

#include "configuration/build/model.h"
#include "configuration/model.h"


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
               const std::vector< configuration::build::model::Resource>& resources, 
               const std::vector< std::string>& keys,
               const configuration::model::system::Model& system);


            inline std::vector< model::Resource> resources( 
               const configuration::build::server::Model& model, 
               const std::vector< std::string>& keys,
               const configuration::model::system::Model& system)
            {
               return resources( model.server.resources, keys, system);
            }


            std::vector< model::Service> services( 
               const configuration::build::server::Model& model, 
               const std::vector< std::string>& names,
               const std::string& transaction_mode);
         } // transform
      } // build
   } // tools
} // casual