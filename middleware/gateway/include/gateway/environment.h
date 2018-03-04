//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_ENVIRONMENT_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_ENVIRONMENT_H_


#include "common/uuid.h"
#include "common/platform.h"
#include "common/communication/ipc.h"

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
                  const std::string& queue();
               } // manager


            } // name

         } // variable

      } // environment
   } // gateway


} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_ENVIRONMENT_H_
