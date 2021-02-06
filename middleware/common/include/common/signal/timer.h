//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"
#include "common/move.h"
#include "common/value/optional.h"

namespace casual
{
   namespace common::signal::timer
   {
      namespace unit
      {
         namespace detail
         {
            struct policy
            {
               constexpr static auto initialize() noexcept { return platform::time::unit::min();}
               constexpr static bool empty( platform::time::unit value) noexcept { return value == platform::time::unit::min();}
            };
         } // detail

         //using type = value::basic_optional< platform::time::unit, detail::policy>; 
         using type = std::optional< platform::time::unit>;
      } // unit

      namespace point
      {
         using type = std::optional< platform::time::point::type>;
      } // point

      //! Sets a timeout.
      //!
      //! @param offset when the timer kicks in.
      //! @returns previous timeout.
      //!
      //! @note zero and negative offset will trigger a signal directly
      //! @note 'empty' offset will unset current timeout, if any.
      unit::type set( unit::type offset);

      template< typename R, typename P>
      unit::type set( std::chrono::duration< R, P> offset)
      {
         return set( unit::type{ std::chrono::duration_cast< platform::time::unit>( offset)});
      }

      //! sets a timout that will expire ot `deadline`
      template< typename R, typename P>
      unit::type set( std::chrono::time_point< R, P> deadline)
      {
         return set( deadline - platform::time::point::type::clock::now());
      }

      //! @return current timeout, or 'emtpy' if there isn't one
      unit::type get();

      //! Unset current timeout, if any.
      //!
      //! @return previous timeout, 'empty' if there wasn't one
      unit::type unset();

      //! Sets a scoped timout.
      //! dtor will 'reset' previous timeout, if any. Hence enable nested timeouts.
      class Scoped
      {
      public:

         Scoped( unit::type timeout);
         Scoped( unit::type timeout, platform::time::point::type now);

         template< typename R, typename P>
         Scoped( std::chrono::duration< R, P> timeout)
            : Scoped( unit::type{ std::chrono::duration_cast< platform::time::unit>( timeout)})
         {}

         ~Scoped();

         Scoped( Scoped&&) noexcept;
         Scoped& operator = ( Scoped&&) noexcept;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_active, "active");
            CASUAL_SERIALIZE_NAME( m_old, "old");
         )

      private:
         move::Active m_active;
         point::type m_old;
      };

      //! Sets a scoped Deadline.
      //! dtor will 'unset' timeout regardless
      class Deadline
      {
      public:

         Deadline( point::type deadline, platform::time::point::type now);
         ~Deadline();

         Deadline( Deadline&&) noexcept;
         Deadline& operator = ( Deadline&&) noexcept;

      private:
         move::Active m_active;
      };

   } // common::signal::timer
} // casual