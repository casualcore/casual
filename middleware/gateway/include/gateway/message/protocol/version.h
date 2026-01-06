//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include "common/array.h"
#include "common/algorithm.h"

#include <string_view>

namespace casual
{
   namespace gateway::message::protocol
   {
      enum struct Version : platform::size::type
      {
         invalid = 0,
         v1_0 = 1000,
         v1_1 = 1001,
         v1_2 = 1002,
         v1_3 = 1003,
         v1_4 = 1004,
         v1_5 = 1005,
         v1_6 = 1006,
         current = v1_6,
      };

      constexpr std::string_view description( Version value) noexcept
      {
         switch( value)
         {
            case Version::invalid: return "invalid";
            case Version::v1_0: return "1.0";
            case Version::v1_1: return "1.1";
            case Version::v1_2: return "1.2";
            case Version::v1_3: return "1.3";
            case Version::v1_4: return "1.4";
            case Version::v1_5: return "1.5";
            case Version::v1_6: return "1.6";
         };
         return "<unknown>";
      }

      //! an array with all versions ordered by highest to lowest
      constexpr auto versions = common::array::make( Version::v1_6, Version::v1_5, Version::v1_4, Version::v1_3, Version::v1_2, Version::v1_1, Version::v1_0);

      //! @returns the version we're using. Version::current, unless it's overridden
      Version version();

   } // gateway::message::protocol
   
} // casual
