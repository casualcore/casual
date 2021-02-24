//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/discovery/common.h"

namespace casual
{
   namespace domain::discovery
   {

      common::log::Stream log{ "casual.domain.discovery"};

      namespace trace
      {
         common::log::Stream log{ "casual.domain.discovery.trace"};
      } // trace

      namespace verbose
      {
         common::log::Stream log{ "casual.domain.discovery.verbose"};
      } // verbose

   } // domain::discovery
} // casual


