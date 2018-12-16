//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "tools/build/resource.h"



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

            struct Content 
            {
               std::string before_main;
               std::string inside_main;
            };


            Content resources( const std::vector< Resource>& resources);

         } // generate
      } // build
   } // tools
} // casual