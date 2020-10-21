//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/executable.h"
#include "common/serialize/macro.h"
#include <optional>

namespace casual
{
   namespace configuration
   {
      struct Server : Executable
      {
         std::optional< std::vector< std::string>> restrictions;
         std::optional< std::vector< std::string>> resources;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            Executable::serialize( archive);
            CASUAL_SERIALIZE( restrictions);
            CASUAL_SERIALIZE( resources);
         )
      };

   } // configuration
} // casual