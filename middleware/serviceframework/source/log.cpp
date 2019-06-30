//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/log.h"


namespace casual
{

   namespace serviceframework
   {
      namespace log
      {
         common::log::Stream debug{ "casual.sf"};
      } // log

      namespace trace
      {
         common::log::Stream log{ "casual.sf.trace"};
      } // trace


   } // serviceframework
} // casual
