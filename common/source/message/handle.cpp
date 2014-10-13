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

            void Terminate::dispatch( message_type& message)
            {
               throw exception::signal::Terminate{};
            }


         } // handle
      } // message
   } // common
} // casual
