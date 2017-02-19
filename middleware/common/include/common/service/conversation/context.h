//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_


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

            private:
               Context();
            };

         } // conversation

      } // service
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_
