//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/resource/property.h"
#include "configuration/build/resource.h"

#include <vector>
#include <string>

namespace casual
{
   namespace tools
   {
      namespace build
      {
         struct Resource
         {
            std::string key;
            std::string name;
            std::string xa_struct_name;

            std::vector< std::string> libraries;

            struct
            {
               std::vector< std::string> include;
               std::vector< std::string> library;
            } paths;
         };

         namespace transform
         {
            std::vector< Resource> resources( 
               const std::vector< configuration::build::Resource>& resources, 
               const std::vector< configuration::resource::Property>& properties);
         } // resource
      } // build
   } // tools
} // casual