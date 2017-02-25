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


            descriptor::type Context::connect(
                  const std::string& service,
                  platform::buffer::raw::immutable::type data,
                  platform::buffer::raw::size::type size,
                  long flags)
            {
               return 0;
            }

         } // conversation

      } // service
   } // common

} // casual
