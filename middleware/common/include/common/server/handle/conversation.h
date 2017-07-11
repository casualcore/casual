//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_HANDLE_CONVERSATION_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_HANDLE_CONVERSATION_H_

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
               using message_type = message::conversation::connect::callee::Request;

               void operator () ( message_type& messaage);
            };

         } // handle
      } // server
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_HANDLE_CONVERSATION_H_
