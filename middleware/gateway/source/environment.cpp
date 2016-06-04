//!
//! casual 
//!

#include "gateway/environment.h"
#include "common/environment.h"


namespace casual
{
   namespace gateway
   {
      namespace environment
      {

         namespace variable
         {
            namespace name
            {
               namespace manager
               {

                  const std::string& queue()
                  {
                     static std::string singleton{ "CASUAL_GATEWAY_MANAGER_QUEUE"};
                     return singleton;
                  }
               } // manager


            } // name

         } // variable




      } // environment
   } // gateway


} // casual
