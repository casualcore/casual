//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"
#include "common/move.h"
#include "common/chronology.h"
#include "common/serialize/macro.h"

#include <optional>

namespace casual
{
   namespace common::signal::timer
   {
      namespace unit
      {
         //using type = value::basic_optional< chronology::duration, detail::policy>; 
         using type = std::optional< chronology::duration>;
      } // unit

      namespace point
      {
         using type = std::optional< chronology::time_point>;
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
         return set( unit::type{ std::chrono::duration_cast< chronology::duration>( offset)});
      }

      //! sets a timout that will expire ot `deadline`
      template< typename R, typename P>
      unit::type set( std::chrono::time_point< R, P> deadline)
      {
         return set( deadline - chronology::time_point::clock::now());
      }

      //! @return current timeout, or 'emtpy' if there isn't one
      unit::type get();

      //! Unset current timeout, if any.
      //!
      //! @return previous timeout, 'empty' if there wasn't one
      unit::type unset();

      //! Sets a scoped timout.
      //! dtor will 'reset' previous timeout, if any. Hence enable nested timeouts.
      struct Scoped
      {
         Scoped( unit::type timeout);
         Scoped( unit::type timeout, chronology::time_point now);

         template< typename R, typename P>
         Scoped( std::chrono::duration< R, P> timeout)
            : Scoped( unit::type{ std::chrono::duration_cast< chronology::duration>( timeout)})
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
      struct Deadline
      {
         Deadline( point::type deadline, chronology::time_point now);
         Deadline( chronology::duration duration);
         ~Deadline();

         Deadline( Deadline&&) noexcept;
         Deadline& operator = ( Deadline&&) noexcept;

         friend std::ostream& operator << ( std::ostream& out, const Deadline& value);

      private:
         move::Active m_active;
      };

   } // common::signal::timer
} // casual