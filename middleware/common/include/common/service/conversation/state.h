//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_STATE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_STATE_H_

#include "common/platform.h"
#include "common/service/descriptor.h"

#include "common/message/conversation.h"

namespace casual
{

   namespace common
   {
      namespace service
      {
         namespace conversation
         {
            namespace state
            {
               namespace descriptor
               {
                  struct Information
                  {
                     enum class Duplex : char
                     {
                        send,
                        receive,
                        terminated
                     };


                     message::conversation::Route route;

                     Duplex duplex = Duplex::receive;
                     bool initiator = false;

                     friend std::ostream& operator << ( std::ostream& out, const Duplex& value);
                     friend std::ostream& operator << ( std::ostream& out, const Information& value);
                  };

               } // descriptor

            } // state

            struct State
            {
               State();

               using holder_type =  service::descriptor::Holder< state::descriptor::Information>;
               using descriptor_type = typename holder_type::descriptor_type;

               holder_type descriptors;
            };

         } // conversation

      } // service
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_STATE_H_
