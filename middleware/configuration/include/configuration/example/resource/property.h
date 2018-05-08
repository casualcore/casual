//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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


