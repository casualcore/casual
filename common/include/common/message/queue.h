//!
//! queue.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMON_MESSAGE_QUEUE_H_
#define COMMON_MESSAGE_QUEUE_H_

#include "common/message/type.h"
#include "common/platform.h"
#include "common/uuid.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace queue
         {

            struct basic_message
            {
               common::Uuid correlation;
               std::string reply;

               common::platform::time_type avalible;

               std::size_t type;
               common::platform::binary_type payload;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & correlation;
                  archive & reply;
                  archive & avalible;
                  archive & type;
                  archive & payload;
               }
            };



            namespace enqueue
            {
               struct Request : basic_messsage< Type::cQueueEnqueueRequest>
               {
                  using Message = basic_message;

                  server::Id server;
                  Transaction xid;
                  std::size_t queue;

                  Message message;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     archive & server;
                     archive & xid;
                     archive & queue;
                     archive & message;
                  }
               };
            } // enqueue

            namespace dequeue
            {
               struct Request : basic_messsage< Type::cQueueDequeueRequest>
               {
                  server::Id server;
                  Transaction xid;
                  std::size_t queue;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     archive & server;
                     archive & xid;
                     archive & queue;
                  }
               };

               struct Reply : basic_messsage< Type::cQueueDequeueReply>
               {
                  struct Message : basic_message
                  {
                     std::size_t redelivered = 0;
                     common::platform::time_type timestamp;

                     template< typename A>
                     void marshal( A& archive)
                     {
                        basic_message::marshal( archive);

                        archive & redelivered;
                        archive & timestamp;
                     }
                  };

                  std::vector< Message> message;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     archive & message;
                  }
               };

            } // dequeue

            struct Queue
            {
               using id_type = std::size_t;

               id_type id = 0;
               std::string name;
               std::size_t retries = 0;
               id_type error = 0;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & id;
                  archive & name;
                  archive & retries;
                  archive & error;
               }
            };

            struct Information : basic_messsage< Type::cQueueInformation>
            {
               struct Queue : queue::Queue
               {
                  std::size_t messages;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     queue::Queue::marshal( archive);

                     archive & messages;
                  }
               };

               std::vector< Queue> queues;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & queues;
               }

            };

            namespace lookup
            {
               struct Request : basic_messsage< Type::cQueueLookupRequest>
               {
                  server::Id server;
                  std::string name;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     archive & server;
                     archive & name;
                  }
               };

               struct Reply : basic_messsage< Type::cQueueLookupReply>
               {
                  server::Id server;
                  std::size_t queue = 0;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     archive & server;
                     archive & queue;
                  }
               };

            } // lookup

         } // queue
      } // message
   } // common



} // casual

#endif // QUEUE_H_
