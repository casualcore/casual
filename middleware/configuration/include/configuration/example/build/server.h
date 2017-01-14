//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_EXAMPLE_BUILD_SERVER_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_EXAMPLE_BUILD_SERVER_H_

#include "configuration/build/server.h"

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace build
         {
            namespace server
            {
               using model_type = configuration::build::server::Server;

               model_type example();

               void write( const model_type& model, const std::string& file);

               common::file::scoped::Path temporary(const model_type& model, const std::string& extension);

            } // server

         } // build

      } // example
   } // configuration
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_EXAMPLE_RESOURCE_PROPERTY_H_
