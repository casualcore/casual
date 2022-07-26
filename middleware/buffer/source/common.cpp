//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/internal/common.h"

#include "common/log/category.h"

namespace casual
{
   namespace buffer
   {
      common::log::Stream& log = common::log::category::buffer;

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

