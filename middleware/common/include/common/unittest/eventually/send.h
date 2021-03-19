//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc/message.h"
#include "common/serialize/native/complete.h"

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace eventually
         {
            Uuid send( strong::ipc::id destination, communication::ipc::message::Complete&& complete);

            template< typename M>
            Uuid send( strong::ipc::id destination, M&& message)
            {
               return send( destination, serialize::native::complete< communication::ipc::message::Complete>( std::forward< M>( message)));
            }
               
         } // eventually
      } // unittest
   } // common
} // casual