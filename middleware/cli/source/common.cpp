//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/cli/common.h"

namespace casual
{
   namespace cli
   {
      common::log::Stream log{ "casual.cli"};

      namespace verbose
      {
         common::log::Stream log{ "casual.cli.verbose"};
      } // verbose

   } // cli
} // casual
