//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "retry/send/common.h"

namespace casual
{
   namespace retry
   {
      namespace send
      {

         common::log::Stream log{ "casual.retry"};

         namespace trace
         {
            common::log::Stream log{ "casual.retry.trace"};
         } // trace

         namespace verbose
         {
            common::log::Stream log{ "casual.retry.verbose"};
         } // verbose
      } // send
      
   } // retry

} // casual




