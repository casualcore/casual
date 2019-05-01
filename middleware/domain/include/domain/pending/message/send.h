//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/message/pending.h"

#include "common/communication/message.h"
#include "common/marshal/complete.h"

namespace casual
{
   namespace domain
   {
      namespace pending
      {
         namespace message
         {
            void send( const common::message::pending::Message& message);

            template< typename M>
            void send( const common::process::Handle& destination, M&& message)
            {
               message::send( common::message::pending::Message{ std::forward< M>( message), destination});
            }
         } // message
         
      } // pending
   } // domain
} // casual