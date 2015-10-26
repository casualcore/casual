//!
//! timeout.h
//!
//! Created on: Jul 12, 2015
//!     Author: Lazan
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
         Timeout( platform::time_point start, std::chrono::microseconds timeout);

         void set( platform::time_point start, std::chrono::microseconds timeout);

         platform::time_point deadline() const;

         platform::time_point start;
         std::chrono::microseconds timeout;

         friend std::ostream& operator << ( std::ostream& out, const Timeout& rhs);
      };

   } // common
} // casual

#endif // TIMEOUT_H_
