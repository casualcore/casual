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
         namespace ipc
         {
            common::log::Stream log{ "casual.ipc"};
         } // ipc

         namespace tcp
         {
            common::log::Stream log{ "casual.tcp"};
         } // ipc

      } // communication
   } // common
} // casual

