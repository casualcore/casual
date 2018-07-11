//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/mockup/log.h"


namespace casual
{
   namespace common
   {
      namespace mockup
      {

         common::log::Stream log{ "casual.mockup"};

         common::log::Stream trace{ "casual.mockup.trace"};

      } // mockup
   } // common
} // casual
