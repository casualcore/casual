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

         // We dont't need async right now, and there are some problems to make sure the user fetch the reply
         // before commit. Pretty much the same problem as in tpacall, but it's somewhat complicated.
         /*
         namespace async
         {

            namespace enqueue
            {
               struct descriptor_t
               {
                  sf::platform::Uuid id;
               };

               descriptor_t request( const std::string& queue, const Message& message);
               sf::platform::Uuid reply( const descriptor_t& descriptor);

            } // enqueue

            namespace dequeue
            {
               struct descriptor_t
               {
                  sf::platform::Uuid id;
               };

               descriptor_t reqeust( const std::string& queue, const Selector& selector);
               std::vector< Message> reply( const descriptor_t& descriptor);
            } // dequeue
         } // async
         */

         namespace blocking
         {
            Message dequeue( const std::string& queue);
            Message dequeue( const std::string& queue, const Selector& selector);
         } // blocking

         namespace peek
         {
            //std::vector< queue::peek::Message> queue( const std::string& queue);

         } // peek

      } // rm


   } // queue
} // casual

#endif // QUEUE_H_
