//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/communication/log.h"


namespace casual
{
   namespace common
   {
      namespace communication
      {
         common::log::Stream log{ "casual.communication"};
         
         namespace verbose
         {
            common::log::Stream log{ "casual.communication.verbose"};
         } // verbose

         namespace trace
         {
            common::log::Stream log{ "casual.communication.trace"};
         } // trace

      } // communication
   } // common
} // casual

