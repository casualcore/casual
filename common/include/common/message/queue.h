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

            struct base_message
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
               struct Request : basic_message< Type::cQueueEnqueueRequest>
               {
                  using Message = base_message;

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
               struct Request : basic_message< Type::cQueueDequeueRequest>
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

               struct Reply : basic_message< Type::cQueueDequeueReply>
               {
                  struct Message : base_message
                  {
                     std::size_t redelivered = 0;
                     common::platform::time_type timestamp;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_message::marshal( archive);

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
               enum Type
               {
                  cGroupErrorQueue = 1,
                  cErrorQueue,
                  cQueue,
               };

               using id_type = std::size_t;

               Queue() = default;
               Queue( std::string name, std::size_t retries) : name( std::move( name)), retries( retries) {}
               Queue( std::string name) : Queue( std::move( name), 0) {};

               id_type id = 0;
               std::string name;
               std::size_t retries = 0;
               id_type error = 0;
               int type;



               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & id;
                  archive & name;
                  archive & retries;
                  archive & error;
                  archive & type;
               })
            };






            namespace information
            {

               struct Queue : message::queue::Queue
               {
                  std::size_t messages;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     message::queue::Queue::marshal( archive);
                     archive & messages;
                  })
               };

               template< message::Type type>
               struct basic_information : basic_message< type>
               {

                  server::Id server;
                  std::vector< Queue> queues;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & server;
                     archive & queues;
                  })

               };

               struct Message
               {
                  common::Uuid id;
                  std::size_t type;
                  std::size_t state;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & id;
                     archive & type;
                     archive & state;
                  })
               };


               namespace queues
               {

                  using Request = server::basic_id< Type::cQueueQueuesInformationRequest>;

                  using Reply = basic_information< Type::cQueueQueuesInformationReply>;

               } // queues

               namespace queue
               {
                  struct Request : server::basic_id< Type::cQueueQueueInformationRequest>
                  {
                     using base_type = server::basic_id< Type::cQueueQueueInformationRequest>;

                     std::string qname;
                     Queue::id_type qid = 0;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & qname;
                        archive & qid;
                     })

                  };

                  struct Reply : common::message::server::basic_id< Type::cQueueQueueInformationReply>
                  {
                     using base_type = common::message::server::basic_id< Type::cQueueQueueInformationReply>;

                     std::vector< Message> messages;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & messages;
                     })

                  };

               } // queue

            } // information

            using Information = information::basic_information< Type::cQueueInformation>;


            namespace lookup
            {
               struct Request : basic_message< Type::cQueueLookupRequest>
               {
                  server::Id server;
                  std::string name;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & server;
                     archive & name;
                  })
               };

               struct Reply : basic_message< Type::cQueueLookupReply>
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
               struct Request : basic_message< Type::cQueueConnectRequest>
               {
                  server::Id server;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & server;
                  })
               };

               struct Reply : basic_message< Type::cQueueConnectReply>
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
               struct Involved : basic_message< Type::cQueueGroupInvolved>
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
