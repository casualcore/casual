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
#include "common/marshal/marshal.h"
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

               std::string properties;
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

               friend std::ostream& operator << ( std::ostream& out, const base_message& value);
            };

            namespace lookup
            {
               struct Request : basic_message< Type::cQueueLookupRequest>
               {
                  process::Handle process;
                  std::string name;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
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
                     base_type::marshal( archive);
                     archive & process;
                     archive & trid;
                     archive & queue;
                     archive & message;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };

               struct Reply : basic_message< Type::cQueueEnqueueReply>
               {
                  common::Uuid id;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & id;
                  })
               };

            } // enqueue

            namespace dequeue
            {
               struct Selector
               {
                  std::string properties;
                  common::Uuid id;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & properties;
                     archive & id;
                  })
                  friend std::ostream& operator << ( std::ostream& out, const Selector& value);
               };

               struct Request : basic_message< Type::cQueueDequeueRequest>
               {
                  process::Handle process;
                  common::transaction::ID trid;
                  std::size_t queue;
                  Selector selector;
                  bool block = false;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & process;
                     archive & trid;
                     archive & queue;
                     archive & selector;
                     archive & block;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };

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
                     base_type::marshal( archive);
                     archive & message;
                  })
               };

               namespace forget
               {
                  struct Request : basic_message< Type::cQueueDequeueForgetRequest>
                  {
                     process::Handle process;
                     std::size_t queue;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & queue;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };

                  struct Reply : basic_message< Type::cQueueDequeueForgetReply>
                  {
                     bool found = false;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & found;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };

               } // forget


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

               friend std::ostream& operator << ( std::ostream& out, const Queue& value);
            };






            namespace information
            {

               struct Queue : message::queue::Queue
               {
                  std::size_t count;
                  std::size_t size;
                  std::size_t uncommitted;
                  platform::time_point timestamp;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     message::queue::Queue::marshal( archive);
                     archive & count;
                     archive & size;
                     archive & uncommitted;
                     archive & timestamp;
                  })
               };

               template< message::Type type>
               struct basic_information : basic_message< type>
               {

                  process::Handle process;
                  std::vector< Queue> queues;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_message< type>::marshal( archive);
                     archive & process;
                     archive & queues;
                  })

               };

               struct Message
               {
                  common::Uuid id;
                  std::size_t queue;
                  std::size_t origin;
                  platform::binary_type trid;
                  std::size_t state;
                  std::string reply;
                  std::size_t redelivered;

                  buffer::Type type;

                  platform::time_point avalible;
                  platform::time_point timestamp;

                  std::size_t size;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & id;
                     archive & queue;
                     archive & origin;
                     archive & trid;
                     archive & state;
                     archive & reply;
                     archive & redelivered;
                     archive & type;
                     archive & avalible;
                     archive & timestamp;
                     archive & size;
                  })
               };


               namespace queues
               {

                  using Request = server::basic_id< Type::cQueueQueuesInformationRequest>;

                  using Reply = basic_information< Type::cQueueQueuesInformationReply>;

               } // queues

               namespace messages
               {
                  struct Request : server::basic_id< Type::cQueueQueueInformationRequest>
                  {
                     using base_type = server::basic_id< Type::cQueueQueueInformationRequest>;

                     Queue::id_type qid = 0;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
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

               } // messages

            } // information

            using Information = information::basic_information< Type::cQueueInformation>;




            namespace connect
            {
               struct Request : basic_message< Type::cQueueConnectRequest>
               {
                  process::Handle process;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & process;
                  })
               };

               struct Reply : basic_message< Type::cQueueConnectReply>
               {
                  std::string name;
                  std::vector< Queue> queues;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
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

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
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
