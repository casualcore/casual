//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/common.h"


namespace casual
{

   namespace domain
   {
      common::log::Stream log{ "casual.domain"};

      namespace trace
      {
         common::log::Stream log{ "casual.domain.trace"};
      } // trace

      namespace verbose
      {
         common::log::Stream log{ "casual.domain.verbose"};
      } // verbose

   } // domain

} // casual
