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
#include "common/buffer/type.h"

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

               common::platform::time_point avalible;

               common::buffer::Type type;
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

            namespace lookup
            {
               struct Request : basic_message< Type::cQueueLookupRequest>
               {
                  process::Handle process;
                  std::string name;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & process;
                     archive & name;
                  })
               };

               struct base_reply
               {
                  base_reply() = default;
                  base_reply( process::Handle process, std::size_t queue) : process( std::move( process)), queue( queue) {}

                  process::Handle process;
                  std::size_t queue = 0;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     archive & process;
                     archive & queue;
                  })
               };

               using Reply = message::type_wrapper< base_reply, Type::cQueueLookupReply>;

            } // lookup



            namespace enqueue
            {
               struct Request : basic_message< Type::cQueueEnqueueRequest>
               {
                  using Message = base_message;

                  process::Handle process;
                  common::transaction::ID trid;
                  std::size_t queue;

                  Message message;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & process;
                     archive & trid;
                     archive & queue;
                     archive & message;
                  })
               };

               struct Reply : basic_message< Type::cQueueEnqueueReply>
               {
                  common::Uuid id;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & id;
                  })
               };

            } // enqueue

            namespace dequeue
            {
               struct base_request
               {
                  process::Handle process;
                  common::transaction::ID trid;
                  std::size_t queue;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & process;
                     archive & trid;
                     archive & queue;
                  })
               };

               using Request = message::type_wrapper< base_request, Type::cQueueDequeueRequest>;

               struct Reply : basic_message< Type::cQueueDequeueReply>
               {
                  struct Message : base_message
                  {
                     std::size_t redelivered = 0;
                     common::platform::time_point timestamp;

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

               namespace callback
               {
                  using Request = message::type_wrapper< base_request, Type::cQueueDequeueCallbackRequest>;

                  using Reply = message::type_wrapper< queue::lookup::base_reply, Type::cQueueDequeueCallbackReply>;

               } // callback

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

                  process::Handle process;
                  std::vector< Queue> queues;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & process;
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




            namespace connect
            {
               struct Request : basic_message< Type::cQueueConnectRequest>
               {
                  process::Handle process;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & process;
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
                  process::Handle process;
                  common::transaction::ID trid;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     archive & process;
                     archive & trid;
                  })

               };

            } // group


         } // queue
      } // message
   } // common



} // casual

#endif // QUEUE_H_
