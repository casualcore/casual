//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "eventually/common.h"

namespace casual
{
   namespace eventually
   {
      common::log::Stream log{ "casual.eventually"};

      namespace trace
      {
         common::log::Stream log{ "casual.eventually.trace"};
      } // trace

      namespace verbose
      {
         common::log::Stream log{ "casual.eventually.verbose"};
      } // verbose
      
   } // eventually

} // casual




