//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_EXAMPLE_RESOURCE_PROPERTY_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_EXAMPLE_RESOURCE_PROPERTY_H_

#include "configuration/resource/property.h"

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace resource
         {
            namespace property
            {
               using resources_type = std::vector< configuration::resource::Property>;

               resources_type example();

               void write( const resources_type& resurces, const std::string& file);

               common::file::scoped::Path temporary(const resources_type& resources, const std::string& extension);

            } // property

         } // resource

      } // example
   } // configuration
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_EXAMPLE_RESOURCE_PROPERTY_H_
