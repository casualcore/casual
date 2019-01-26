//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/common.h"

namespace casual
{
   namespace configuration
   {
      common::log::Stream log{ "casual.configuration"};

      namespace verbose
      {
         common::log::Stream log{ "casual.configuration.verbose"};
      } // verbose

      namespace trace
      {
         common::log::Stream log{ "casual.configuration.trace"};
      } // trace
   } // configuration
} // casual
