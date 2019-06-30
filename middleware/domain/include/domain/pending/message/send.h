//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/message/pending.h"
#include "common/communication/message.h"

namespace casual
{
   namespace domain
   {
      namespace pending
      {
         namespace message
         {
            void send( const common::message::pending::Message& message, const common::communication::error::type& handler = nullptr);

            template< typename M>
            void send( const common::process::Handle& destination, M&& message, const common::communication::error::type& handler = nullptr)
            {
               message::send( common::message::pending::Message{ std::forward< M>( message), destination}, handler);
            }
         } // message
         
      } // pending
   } // domain
} // casual