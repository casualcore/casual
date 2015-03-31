//!
//! queue.h
//!
//! Created on: Jul 9, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_QUEUE_H_
#define CASUAL_QUEUE_QUEUE_H_


#include "queue/api/message.h"

#include "sf/platform.h"


namespace casual
{
   namespace queue
   {
      namespace rm
      {
         sf::platform::Uuid enqueue( const std::string& queue, const Message& message);

         std::vector< Message> dequeue( const std::string& queue);
         std::vector< Message> dequeue( const std::string& queue, const Selector& selector);

         namespace peek
         {
            //std::vector< queue::peek::Message> queue( const std::string& queue);

         } // peek

      } // rm


   } // queue
} // casual

#endif // QUEUE_H_
