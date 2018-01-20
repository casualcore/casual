//!
//! casual 
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

