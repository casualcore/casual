//!
//! Copyright (c) 2022, The casual project
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
      enum class Version : platform::size::type
      {
         invalid = 0,
         v1_0 = 1000,
         v1_1 = 1001,
         v1_2 = 1002,
         v1_3 = 1003,
         current = v1_3,
      };

      constexpr std::string_view description( Version value) noexcept
      {
         switch( value)
         {
            case Version::invalid: return "invalid";
            case Version::v1_0: return "v1.0";
            case Version::v1_1: return "v1.1";
            case Version::v1_2: return "v1.2";
            case Version::v1_3: return "v1.3";
         };
         return "<unknown>";
      }

      //! an array with all versions ordered by highest to lowest
      constexpr auto versions = common::array::make( Version::v1_3, Version::v1_2, Version::v1_1, Version::v1_0);

      consteval Version compiled_for_version()
      {
         #ifdef CASUAL_PROTOCOL_VERSION
            constexpr auto version = protocol::Version{ CASUAL_PROTOCOL_VERSION};

            static_assert( common::algorithm::contains( protocol::versions, version));

            return version;
         #else
            return Version::current;
         #endif
      }


      //! just a helper to make it easier to specialize `version_traits`.
      template< Version MIN, Version MAX = Version::current>
      struct version_helper
      {
         struct Range
         {
            static constexpr Version min = MIN;
            static constexpr Version max = MAX;
         };

         constexpr static auto version() { return Range{};}
      };

      //! traits to be specialized for other versions than v1.0
      template< typename M>
      struct version_traits : version_helper< Version::v1_0, Version::current> {};

      
      //! @returns the version the message needs at least
      template< typename M>
      constexpr auto version()
      {
         return version_traits< std::remove_cvref_t< M>>::version();
      }

      //! @returns true if M (message) is compatible with `current`
      template< typename M>
      constexpr auto compatible( Version current)
      {
         auto range = protocol::version< M>();
         return current >= range.min && current <= range.max;
      }

      //! @returns true if message is compatible with `current`
      template< typename M>
      constexpr auto compatible( M&& message, Version current)
      {
         return protocol::compatible< std::decay_t< M>>( current);
      }

   } //gateway::message::protocol
} // casual