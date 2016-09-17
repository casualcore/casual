//!
//! queue.cpp
//!
//! Created on: May 2, 2015
//!     Author: Lazan
//!

#include "common/message/queue.h"

#include "common/chronology.h"


namespace casual
{
   namespace common
   {

      namespace message
      {
         namespace queue
         {

            std::ostream& operator << ( std::ostream& out, const base_message& value)
            {
               return out << "{ id: " << value.id
                     << ", type: " << value.type
                     << ", properties: " << value.properties
                     << ", reply: " << value.reply
                     << ", available: " << common::chronology::local( value.avalible)
                     << ", size: " << value.payload.size()
                     << '}';
            }

            namespace enqueue
            {
               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ correlation: " << value.correlation
                        << ", process: " << value.process
                        << ", trid: " << value.trid
                        << ", queue: " << value.queue
                        << ", message: " << value.message
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ id: " << value.id
                        << '}';
               }

            } // enqueue

            namespace dequeue
            {
               std::ostream& operator << ( std::ostream& out, const Selector& value)
               {
                  return out << "{ id: " << value.id
                        << ", properties: " << value.properties
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ qid: " << value.queue
                     << ", block: " << std::boolalpha << value.block
                     << ", selector: " << value.selector
                     << ", process: " << value.process
                     << ", trid: " << value.trid << '}';
               }


               namespace forget
               {

                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ correlation: " << value.correlation
                        << ", process: " << value.process
                        << ", queue: " << value.queue << '}';
                  }


                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{ correlation: " << value.correlation
                        << ", found: " << value.found << '}';
                  }

               } // forget
            } // dequeue

            std::ostream& operator << ( std::ostream& out, const Queue& value)
            {
               return out << "{ id: " << value.id
                     << ", name: " << value.name
                     << ", type: " << value.type
                     << ", retries: " << value.retries
                     << ", error: " << value.error
                     << '}';

            }

         } // queue
      } // message
   } // common
} // casual
