//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "casual/platform.h"
#include "common/serialize/macro.h"
#include "common/signal/timer.h"

#include <iosfwd>
#include <chrono>


namespace casual
{
   namespace common
   {
      struct Timeout
      {

         Timeout() = default;
         Timeout( platform::time::point::type start, platform::time::unit timeout);

         void set( platform::time::point::type start, platform::time::unit timeout);

         signal::timer::point::type deadline() const;

         inline auto timeout() const noexcept { return m_timeout;}

         CASUAL_LOG_SERIALIZE({
            CASUAL_SERIALIZE_NAME( m_start, "start");
            CASUAL_SERIALIZE_NAME( m_timeout, "timeout");
         })

      private:
         platform::time::point::type m_start{};
         platform::time::unit m_timeout{};
      };

   } // common
} // casual


