//!
//! casual
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

