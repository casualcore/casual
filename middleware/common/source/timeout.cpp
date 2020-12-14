//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/timeout.h"

#include "common/log.h"
#include "common/chronology.h"

#include <ostream>

namespace casual
{
   namespace common
   {

      Timeout::Timeout( platform::time::point::type start, platform::time::unit timeout)
         : m_start{ start}, m_timeout{ timeout} {}

      void Timeout::set( platform::time::point::type start, platform::time::unit timeout)
      {
         m_start = start;
         m_timeout = timeout;
      }

      signal::timer::point::type Timeout::deadline() const
      {
         if( m_timeout == platform::time::unit::zero())
            return {};

         return m_start + m_timeout;
      }

   } // common
} // casual
