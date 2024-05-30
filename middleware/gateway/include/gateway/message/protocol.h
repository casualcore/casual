//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include "common/array.h"

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

      std::string_view description( Version value) noexcept;

      //! an array with all versions ordered by highest to lowest
      constexpr auto versions = common::array::make( Version::v1_2, Version::v1_1, Version::v1_0);

      consteval Version compiled_for_version()
      {
         #if CASUAL_PROTOCOL_VERSION == 1002
            return Version::v1_2;
         #else
            return Version::current;
         #endif
      }

      //! just a helper to make it easier to specialize `version_traits`.
      template< Version value>
      struct version_helper
      {
         constexpr static auto version() { return value;}
      };

      //! traits to be specialized for other versions than v1.0
      template< typename M>
      struct version_traits : version_helper< Version::v1_0> {};

      
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
         return current >= protocol::version< M>();
      }

      //! @returns true if message is compatible with `current`
      template< typename M>
      constexpr auto compatible( M&& message, Version current)
      {
         return protocol::compatible< std::decay_t< M>>( current);
      }

   } //gateway::message::protocol
} // casual