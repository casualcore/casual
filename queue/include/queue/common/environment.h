//!
//! environment.h
//!
//! Created on: Jul 9, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_ENVIRONMENT_H_
#define CASUAL_QUEUE_ENVIRONMENT_H_


#include "common/platform.h"

#include <string>



namespace casual
{
   namespace queue
   {
      namespace environment
      {
         namespace broker
         {
            namespace queue
            {
               std::string path();

               common::platform::queue_id_type id();

            } // queue

         } // broker

      } // environment
   } // queue
} // casual

#endif // ENVIRONMENT_H_
