//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "xatmi/internal/log.h"

namespace casual
{
   namespace xatmi
   {
      common::log::Stream log{ "xatmi"};

      namespace trace
      {
         common::log::Stream log{ "xatmi.trace"};
      } // trace

   } // xatmi
} // casual