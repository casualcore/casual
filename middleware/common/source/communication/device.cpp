//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/device.h"

#include "common/signal.h"
#include "common/code/casual.h"


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
                  catch( ...)
                  {
                     if( exception::capture().code() != code::casual::interrupted)
                        throw;

                     log::line( verbose::log, "device interrupted");
                     signal::dispatch();
                  }
               }
            } // handle
         } // device
      } // communication
   } // common
} // casual