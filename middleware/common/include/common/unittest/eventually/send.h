//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/message.h"
#include "common/marshal/binary.h"
#include "common/marshal/complete.h"

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace eventually
         {
            Uuid send( strong::ipc::id destination, communication::message::Complete&& complete);

            template< typename M, typename C = marshal::binary::create::Output>
            Uuid send( strong::ipc::id destination, M&& message, C creator = marshal::binary::create::Output{})
            {
               return send( destination, marshal::complete( std::forward< M>( message), creator));
            }
               
         } // eventually
      } // unittest
   } // common
} // casual