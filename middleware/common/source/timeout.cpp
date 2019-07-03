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

      Timeout::Timeout() : start{ platform::time::point::limit::zero()}, timeout{ 0} {}

      Timeout::Timeout( platform::time::point::type start, common::platform::time::unit timeout)
         : start{ std::move( start)}, timeout{ timeout} {}

      void Timeout::set( platform::time::point::type start_, common::platform::time::unit timeout_)
      {
         start = std::move( start_);
         timeout = timeout_;
      }

      platform::time::point::type Timeout::deadline() const
      {
         if( timeout == common::platform::time::unit::zero())
         {
            return platform::time::point::type::max();
         }
         return start + timeout;
      }

      std::ostream& operator << ( std::ostream& out, const Timeout& rhs)
      {
         return stream::write( out, "{ start: ", rhs.start, ", timeout: ", rhs.timeout, "}");
      }

   } // common
} // casual
