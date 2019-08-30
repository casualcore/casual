//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/message.h"
#include "common/serialize/native/complete.h"

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace eventually
         {
            Uuid send( strong::ipc::id destination, communication::message::Complete&& complete);

            template< typename M, typename C = serialize::native::binary::create::Writer>
            Uuid send( strong::ipc::id destination, M&& message, C creator = serialize::native::binary::create::Writer{})
            {
               return send( destination, serialize::native::complete( std::forward< M>( message), creator));
            }
               
         } // eventually
      } // unittest
   } // common
} // casual