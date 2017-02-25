//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_

#include "common/service/conversation/state.h"

namespace casual
{

   namespace common
   {
      namespace service
      {
         namespace conversation
         {
            class Context
            {
            public:
               static Context& instance();

               descriptor::type connect( const std::string& service, platform::buffer::raw::immutable::type data, platform::buffer::raw::size::type size, long flags);

            private:
               Context();
            };

         } // conversation

      } // service
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_
