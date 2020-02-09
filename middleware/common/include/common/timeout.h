//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "casual/platform.h"
#include "common/serialize/macro.h"

#include <iosfwd>
#include <chrono>


namespace casual
{
   namespace common
   {

      struct Timeout
      {

         Timeout();
         Timeout( platform::time::point::type start, platform::time::unit timeout);

         void set( platform::time::point::type start, platform::time::unit timeout);

         platform::time::point::type deadline() const;

         platform::time::point::type start;
         platform::time::unit timeout;
         
         CASUAL_CONST_CORRECT_SERIALIZE_WRITE({
            CASUAL_SERIALIZE( start);
            CASUAL_SERIALIZE( timeout);
         })
      };

   } // common
} // casual


