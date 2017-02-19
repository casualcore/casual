//!
//! casual 
//!

#include "common/service/conversation/context.h"

namespace casual
{

   namespace common
   {
      namespace service
      {
         namespace conversation
         {
            Context& Context::instance()
            {
               static Context singleton;
               return singleton;
            }

            Context::Context() = default;

         } // conversation

      } // service
   } // common

} // casual
