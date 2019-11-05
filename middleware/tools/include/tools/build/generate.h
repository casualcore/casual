//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "tools/build/model.h"



#include <iosfwd>
#include <vector>

namespace casual
{
   namespace tools
   {
      namespace build
      {
         namespace generate
         {
            
            //! generates the server main source file content
            void server( std::ostream& out, 
               const std::vector< model::Resource>& resources, 
               const std::vector< model::Service>& services);


            void executable( std::ostream& out, 
               const std::vector< model::Resource>& resources, 
               const std::string& entrypoint);

         } // generate
      } // build
   } // tools
} // casual