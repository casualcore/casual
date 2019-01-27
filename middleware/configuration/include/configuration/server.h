//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/executable.h"

namespace casual
{
   namespace configuration
   {
      struct Server : Executable
      {
         serviceframework::optional< std::vector< std::string>> restrictions;
         serviceframework::optional< std::vector< std::string>> resources;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            Executable::serialize( archive);
            archive & CASUAL_MAKE_NVP( restrictions);
            archive & CASUAL_MAKE_NVP( resources);
         )
      };

   } // configuration
} // casual