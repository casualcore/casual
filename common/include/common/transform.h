//!
//! transform.h
//!
//! Created on: Dec 4, 2012
//!     Author: Lazan
//!

#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include "xatmi.h"

#include "common/message.h"


namespace casual
{
   namespace common
   {
      namespace transform
      {


         struct ServiceInformation
         {
            TPSVCINFO operator () ( message::service::callee::Call& message) const
            {
               TPSVCINFO result;

               strncpy( result.name, message.service.name.data(), sizeof( result.name) );
               result.data = platform::public_buffer( message.buffer.raw());
               result.len = message.buffer.size();
               result.cd = message.callDescriptor;
               result.flags = 0;

               return result;
            }

         };

      }
   }

}



#endif /* TRANSFORM_H_ */
