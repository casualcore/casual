//!
//! casual
//!

#include "common/log.h"

namespace casual
{
   namespace common
   {
      namespace log
      {
         common::log::Stream debug{ "casual.common"};

      } // log

      namespace trace
      {
         log::Stream log{ "casual.trace"};
      } // trace

      namespace verbose
      {
         log::Stream log{ "casual.common.verbose"};
      } // verbose

   } // common
} // casual


