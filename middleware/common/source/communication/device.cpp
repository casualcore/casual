//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/device.h"

#include "common/exception/system.h"
#include "common/signal.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace device
         {
            namespace handle
            {
               void error()
               {
                  try
                  {
                     throw;
                  }
                  catch( const exception::system::Interupted& exception)
                  {
                     log::line( verbose::log, "device interupted: ", exception);

                     signal::dispatch();
                  }
               }
            } // handle
         } // device
      } // communication
   } // common
} // casual