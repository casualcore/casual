//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/message/queue.h"
#include "common/transcode.h"

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
                     << ", available: " << common::chronology::local( value.available)
                     << ", size: " << value.payload.size()
                     << '}';
            }

            namespace lookup
            {
               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ name: " << value.name
                        << '}';

               }

               bool Reply::local() const
               {
                  return order == 0;
               }

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ process: " << value.process
                        << ", queue: " << value.queue
                        << '}';
               }


            } // lookup

            namespace enqueue
            {
               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ correlation: " << value.correlation
                        << ", process: " << value.process
                        << ", trid: " << value.trid
                        << ", queue: " << value.queue
                        << ", name: " << value.name
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
                  return out << "{ name: " << value.name
                        << ", queue: " << value.queue
                        << ", block: " << std::boolalpha << value.block
                        << ", selector: " << value.selector
                        << ", process: " << value.process
                        << ", trid: " << value.trid << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ message: " << range::make( value.message)
                        << '}';
               }

               namespace forget
               {

                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ correlation: " << value.correlation
                        << ", process: " << value.process
                        << ", queue: " << value.queue
                        << ", name: " << value.name << '}';
                  }


                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{ correlation: " << value.correlation
                        << ", found: " << value.found << '}';
                  }

               } // forget
            } // dequeue

            namespace peek
            {
               std::ostream& operator << ( std::ostream& out, const Information& value)
               {
                  return out << "{"
                        << '}';
               }

               namespace information
               {
                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{"
                           << '}';
                  }

                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{ messages: " << range::make( value.messages)
                           << '}';
                  }
               } // information

               namespace messages
               {
                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{"
                           << '}';
                  }

                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{"
                           << '}';
                  }
               } // messages

            } // peek

            std::ostream& operator << ( std::ostream& out, const Queue::Type& value)
            {
               switch( value)
               {
                  case Queue::Type::group_error_queue: { return out << "group-error-queue";}
                  case Queue::Type::error_queue: { return out << "error-queue";}
                  case Queue::Type::queue: { return out << "queue";}
               }
               return out << "unknown";
            }

            std::ostream& operator << ( std::ostream& out, const Queue& value)
            {
               return out << "{ id: " << value.id
                     << ", name: " << value.name
                     << ", type: " << value.type
                     << ", retries: " << value.retries
                     << ", error: " << value.error
                     << '}';

            }

            namespace information
            {
               std::ostream& operator << ( std::ostream& out, const Queue& value)
               {
                  return out << "{ id: " << value.id
                     << ", name: " << value.name
                     << ", type: " << value.type
                     << ", retries: " << value.retries
                     << ", error: " << value.error
                     << ", count: " << value.count
                     << ", size: " <<  value.size
                     << ", uncommitted: " << value.uncommitted
                     << ", pending: " << value.pending
                     << ", timestamp: " << value.timestamp.time_since_epoch().count()
                     << '}';
               }
               std::ostream& operator << ( std::ostream& out, const Message& value)
               {
                  return out << "{ id: " << value.id
                        << ", queue: " << value.queue
                        << ", origin: " << value.origin
                        << ", state: " << value.state
                        << ", properties: " << value.properties
                        << ", reply: " << value.reply
                        << ", redelivered: " << value.redelivered
                        << ", trid: " << transcode::hex::encode( value.trid)
                        << ", type: " << value.type
                        << ", size: " << value.size
                        << '}';

               }
            } // information

         } // queue
      } // message
   } // common
} // casual
