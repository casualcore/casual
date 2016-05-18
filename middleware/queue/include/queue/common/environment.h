//!
//! casual
//!

#ifndef CASUAL_QUEUE_ENVIRONMENT_H_
#define CASUAL_QUEUE_ENVIRONMENT_H_


#include "common/platform.h"
#include "common/uuid.h"
#include "common/communication/ipc.h"

#include <string>



namespace casual
{
   namespace queue
   {
      namespace environment
      {
         namespace ipc
         {
            namespace broker
            {
               //!
               //! @return ipc device to queue broker
               //!
               common::communication::ipc::outbound::instance::Device& device();

            } // broker

         } // ipc

         namespace identity
         {
            const common::Uuid& broker();

         } // identity

      } // environment
   } // queue
} // casual

#endif // ENVIRONMENT_H_
