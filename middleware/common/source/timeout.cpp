//!
//! casual
//!

#include "common/timeout.h"
#include "common/chronology.h"

#include <ostream>

namespace casual
{
   namespace common
   {

      Timeout::Timeout() : start{ platform::time::point::type::min()}, timeout{ 0} {}

      Timeout::Timeout( platform::time::point::type start, std::chrono::microseconds timeout)
         : start{ std::move( start)}, timeout{ timeout} {}

      void Timeout::set( platform::time::point::type start_, std::chrono::microseconds timeout_)
      {
         start = std::move( start_);
         timeout = timeout_;
      }

      platform::time::point::type Timeout::deadline() const
      {
         if( timeout == std::chrono::microseconds::zero())
         {
            return platform::time::point::type::max();
         }
         return start + timeout;
      }

      std::ostream& operator << ( std::ostream& out, const Timeout& rhs)
      {
         return out << "{ start: " << chronology::local( rhs.start) << ", timeout: " << rhs.timeout.count() << "us }";
      }

   } // common
} // casual
