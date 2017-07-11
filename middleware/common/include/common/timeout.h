//!
//! casual
//!

#ifndef COMMON_TIMEOUT_H_
#define COMMON_TIMEOUT_H_


#include "common/platform.h"

#include <iosfwd>
#include <chrono>


namespace casual
{
   namespace common
   {

      struct Timeout
      {

         Timeout();
         Timeout( platform::time::point::type start, std::chrono::microseconds timeout);

         void set( platform::time::point::type start, std::chrono::microseconds timeout);

         platform::time::point::type deadline() const;

         platform::time::point::type start;
         std::chrono::microseconds timeout;

         friend std::ostream& operator << ( std::ostream& out, const Timeout& rhs);
      };

   } // common
} // casual

#endif // TIMEOUT_H_
