//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/build/resource.h"

#include "common/serialize/macro.h"

#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace build
      {
         struct Executable
         {
            Executable();
            ~Executable();
            
            std::vector< build::Resource> resources;

            //! name of the function that casual invokes 
            //! after resource setup.
            //! Has to be defined as  int <entrypoint-name>( int argc, const char** argv)
            //!
            //! default: executable_entrypoint
            //!
            std::string entrypoint;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( resources);
               CASUAL_SERIALIZE( entrypoint);
            )

         };

         namespace executable
         {
            Executable get( const std::string& file);
         } // executable

      } // build
   } // configuration
} // casual