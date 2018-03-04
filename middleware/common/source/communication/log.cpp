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

