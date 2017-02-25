//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_STATE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_STATE_H_

#include "common/platform.h"

namespace casual
{

   namespace common
   {
      namespace service
      {
         namespace conversation
         {
            namespace descriptor
            {
               using type = platform::descriptor::type;
            } // descriptor

            namespace state
            {
               namespace descriptor
               {
                  struct Holder
                  {
                     Holder();

                  };

               } // descriptor

            } // state

            struct State
            {


            };

         } // conversation

      } // service
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_STATE_H_
