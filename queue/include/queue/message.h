//!
//! message.h
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "common/platform.h"
#include "common/uuid.h"

namespace casual
{
   namespace queue
   {
      struct Message
      {
         common::Uuid correlation;
         std::size_t type;

         std::string reply;
         std::size_t redelivered = 0;

         common::platform::time_type avalible;
         common::platform::time_type timestamp;
         common::platform::binary_type payload;
      };


   } // queue
} // casual

#endif // MESSAGE_H_
