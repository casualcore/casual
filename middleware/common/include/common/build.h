//! 
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include "common/serialize/macro.h"

#include <string>
#include <vector>

namespace casual
{
   namespace common::build
   {
      struct Version
      {
         std::string casual;
         std::string compiler;
         std::string commit;

         struct
         {
            std::vector< platform::size::type> protocols;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( protocols);
            )
         } gateway;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( casual);
            CASUAL_SERIALIZE( compiler);
            CASUAL_SERIALIZE( commit);
            CASUAL_SERIALIZE( gateway);
         )

      };

      //! @returns the current version of the build
      Version version() noexcept;

   } // common::build
} // casual
