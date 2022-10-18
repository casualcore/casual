//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/buffer/pool.h"


namespace casual
{
   namespace common::buffer
   {
      struct x_octet : pool::implementation::default_buffer
      {
         static constexpr auto types()
         {
            // TODO Why all these? Don't understand my former self...
            return array::make( type::x_octet, type::binary, type::json, type::yaml, type::xml, type::ini);
         }
      };

      // Register the pool to the pool-holder
      template struct pool::Registration< x_octet>;




   } // common::buffer
} // casual
