//!
//! timeout.cpp
//!
//! Created on: Jul 12, 2015
//!     Author: Lazan
//!

#include "common/timeout.h"
#include "common/chronology.h"

#include <ostream>

namespace casual
{
   namespace common
   {

      Timeout::Timeout() : start{ platform::time_point::min()}, timeout{ 0} {}

      Timeout::Timeout( platform::time_point start, std::chrono::microseconds timeout)
         : start{ std::move( start)}, timeout{ timeout} {}

      void Timeout::set( platform::time_point start_, std::chrono::microseconds timeout_)
      {
         start = std::move( start_);
         timeout = timeout_;
      }

      platform::time_point Timeout::deadline() const
      {
         if( timeout == std::chrono::microseconds::zero())
         {
            return platform::time_point::max();
         }
         return start + timeout;
      }

      std::ostream& operator << ( std::ostream& out, const Timeout& rhs)
      {
         return out << "{ start: " << chronology::local( rhs.start) << ", timeout: " << rhs.timeout.count() << "us }";
      }

   } // common
} // casual
