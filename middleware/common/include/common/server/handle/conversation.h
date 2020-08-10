//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/conversation.h"

namespace casual
{
   namespace common
   {
      namespace server
      {
         namespace handle
         {
            struct Conversation
            {
               void operator () ( message::conversation::connect::callee::Request& message);
            };

         } // handle
      } // server
   } // common
} // casual


