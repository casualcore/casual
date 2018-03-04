//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "buffer/internal/common.h"

namespace casual
{
   namespace buffer
   {
      common::log::Stream log{ "casual.buffer"};

      namespace verbose
      {
         common::log::Stream log{ "casual.buffer.verbose"};
      } // verbose

      namespace trace
      {
         common::log::Stream log{ "casual.buffer.trace"};
      } // trace

   } // buffer
} // casual

