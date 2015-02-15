//!
//! handle.cpp
//!
//! Created on: Oct 13, 2014
//!     Author: Lazan
//!

#include "common/message/handle.h"
#include "common/exception.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace handle
         {

            void Shutdown::operator () ( message_type& message)
            {
               throw exception::Shutdown{ "shutdown " + process::path()};
            }


         } // handle
      } // message
   } // common
} // casual
