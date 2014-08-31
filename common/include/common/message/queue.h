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
#include "common/marshal.h"

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
               common::Uuid id;

               std::string correlation;
               std::string reply;

               common::platform::time_type avalible;

               std::size_t type;
               common::platform::binary_type payload;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & id;
                  archive & reply;
                  archive & avalible;
                  archive & type;
                  archive & payload;
               })
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

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & server;
                     archive & xid;
                     archive & queue;
                     archive & message;
                  })
               };
            } // enqueue

            namespace dequeue
            {
               struct Request : basic_messsage< Type::cQueueDequeueRequest>
               {
                  server::Id server;
                  Transaction xid;
                  std::size_t queue;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & server;
                     archive & xid;
                     archive & queue;
                  })
               };

               struct Reply : basic_messsage< Type::cQueueDequeueReply>
               {
                  struct Message : basic_message
                  {
                     std::size_t redelivered = 0;
                     common::platform::time_type timestamp;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        basic_message::marshal( archive);

                        archive & redelivered;
                        archive & timestamp;
                     })
                  };

                  std::vector< Message> message;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & message;
                  })
               };

            } // dequeue

            struct Queue
            {
               using id_type = std::size_t;

               Queue() = default;
               Queue( std::string name, std::size_t retries) : name( std::move( name)), retries( retries) {}

               id_type id = 0;
               std::string name;
               std::size_t retries = 0;
               id_type error = 0;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & id;
                  archive & name;
                  archive & retries;
                  archive & error;
               })
            };

            struct Information : basic_messsage< Type::cQueueInformation>
            {
               struct Queue : queue::Queue
               {
                  std::size_t messages;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     queue::Queue::marshal( archive);
                     archive & messages;
                  })
               };

               server::Id server;
               std::vector< Queue> queues;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & server;
                  archive & queues;
               })

            };

            namespace lookup
            {
               struct Request : basic_messsage< Type::cQueueLookupRequest>
               {
                  server::Id server;
                  std::string name;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & server;
                     archive & name;
                  })
               };

               struct Reply : basic_messsage< Type::cQueueLookupReply>
               {
                  Reply() = default;
                  Reply( server::Id id, std::size_t queue) : server( id), queue( queue) {}

                  server::Id server;
                  std::size_t queue = 0;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     archive & server;
                     archive & queue;
                  })
               };

            } // lookup

            namespace connect
            {
               struct Request : basic_messsage< Type::cQueueConnectRequest>
               {
                  server::Id server;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & server;
                  })
               };

               struct Reply : basic_messsage< Type::cQueueConnectReply>
               {
                  std::string name;
                  std::vector< Queue> queues;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     archive & name;
                     archive & queues;
                  })
               };
            } // connect

            namespace group
            {
               struct Involved : basic_messsage< Type::cQueueGroupInvolved>
               {
                  server::Id server;
                  Transaction xid;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     archive & server;
                     archive & xid;
                  })

               };

            } // group


         } // queue
      } // message
   } // common



} // casual

#endif // QUEUE_H_
