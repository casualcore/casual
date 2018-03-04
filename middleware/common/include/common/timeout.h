//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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
         Timeout( platform::time::point::type start, common::platform::time::unit timeout);

         void set( platform::time::point::type start, common::platform::time::unit timeout);

         platform::time::point::type deadline() const;

         platform::time::point::type start;
         common::platform::time::unit timeout;

         friend std::ostream& operator << ( std::ostream& out, const Timeout& rhs);
      };

   } // common
} // casual

#endif // TIMEOUT_H_
