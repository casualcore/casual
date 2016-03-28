//!
//! casual 
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
         const common::Uuid& identification();

         namespace manager
         {

            common::communication::ipc::outbound::Device& device();

            void set( common::platform::ipc::id::type queue);

         } // manager


      } // environment
   } // gateway


} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_ENVIRONMENT_H_
