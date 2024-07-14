//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/message/protocol/version.h"
#include "gateway/message.h"

namespace casual
{
   namespace gateway::message::protocol
   {
     
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



      template<>
      struct version_traits< domain::disconnect::Request> : version_helper< Version::v1_1> {};

      template<>
      struct version_traits< domain::disconnect::Reply> : version_helper< Version::v1_1> {};

      template<>
      struct version_traits< casual::domain::message::discovery::topology::implicit::Update> : version_helper< Version::v1_2> {};

      template<>
      struct version_traits< casual::queue::ipc::message::group::enqueue::Reply> : version_helper< Version::v1_3> {};

      template<>
      struct version_traits< casual::queue::ipc::message::group::enqueue::v1_2::Reply> : version_helper< Version::v1_0, Version::v1_2> {};

      template<>
      struct version_traits< casual::queue::ipc::message::group::dequeue::Reply> : version_helper< Version::v1_3> {};

      template<>
      struct version_traits< casual::queue::ipc::message::group::dequeue::v1_2::Reply> : version_helper< Version::v1_0, Version::v1_2> {};

      template<>
      struct version_traits< common::message::service::call::callee::Request> : version_helper< Version::v1_3> {};

      template<>
      struct version_traits< common::message::service::call::Reply> : version_helper< Version::v1_3> {};

      template<>
      struct version_traits< common::message::service::call::v1_2::callee::Request> : version_helper< Version::v1_0, Version::v1_2> {};

      template<>
      struct version_traits< common::message::service::call::v1_2::Reply> : version_helper< Version::v1_0, Version::v1_2> {};

      template<>
      struct version_traits< common::message::conversation::connect::callee::Request> : version_helper< Version::v1_3> {};

      template<>
      struct version_traits< common::message::conversation::connect::v1_2::callee::Request> : version_helper< Version::v1_0, Version::v1_2> {};


   } //gateway::message::protocol
} // casual