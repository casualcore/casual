//!
//! environment.h
//!
//! Created on: Jul 9, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_ENVIRONMENT_H_
#define CASUAL_QUEUE_ENVIRONMENT_H_


#include "common/platform.h"
#include "common/uuid.h"

#include <string>



namespace casual
{
   namespace queue
   {
      namespace environment
      {
         namespace broker
         {
            const common::Uuid& identification();

            namespace queue
            {

               common::platform::queue_id_type id();

            } // queue

         } // broker

      } // environment
   } // queue
} // casual

#endif // ENVIRONMENT_H_
