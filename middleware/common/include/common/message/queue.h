//!
//! casual
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
            struct base_message_information
            {
               common::Uuid id;

               std::string properties;
               std::string reply;

               common::platform::time::point::type avalible;
               std::string type;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & id;
                  archive & properties;
                  archive & reply;
                  archive & avalible;
                  archive & type;
               })

            };

            struct base_message : base_message_information
            {
               common::platform::binary::type payload;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  base_message_information::marshal( archive);
                  archive & payload;
               })

               friend std::ostream& operator << ( std::ostream& out, const base_message& value);
            };
            static_assert( traits::is_movable< base_message>::value, "not movable");

            namespace lookup
            {
               struct Request : basic_message< Type::queue_lookup_request>
               {
                  common::process::Handle process;
                  std::string name;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & process;
                     archive & name;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_message< Type::queue_lookup_reply>
               {
                  Reply() = default;
                  Reply( common::process::Handle process) : process( std::move( process)) {}

                  common::process::Handle process;
                  std::size_t queue = 0;
                  std::size_t order = 0;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     archive & process;
                     archive & queue;
                     archive & order;
                  })

                  bool local() const;

                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");
            } // lookup

            namespace enqueue
            {
               struct Request : basic_message< Type::queue_enqueue_request>
               {
                  using Message = base_message;

                  common::process::Handle process;
                  common::transaction::ID trid;
                  long queue = 0;
                  std::string name;

                  Message message;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & process;
                     archive & trid;
                     archive & queue;
                     archive & name;
                     archive & message;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_message< Type::queue_enqueue_reply>
               {
                  common::Uuid id;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & id;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

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

               struct Request : basic_message< Type::queue_dequeue_request>
               {
                  common::process::Handle process;
                  common::transaction::ID trid;
                  std::size_t queue = 0;
                  std::string name;
                  Selector selector;
                  bool block = false;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & process;
                     archive & trid;
                     archive & queue;
                     archive & name;
                     archive & selector;
                     archive & block;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_message< Type::queue_dequeue_reply>
               {
                  struct Message : base_message
                  {
                     Message( const base_message& m) : base_message( m) {}
                     Message() = default;

                     //Message( Message&&) = default;
                     //Message& operator = ( Message&&) = default;


                     long redelivered = 0;
                     common::platform::time::point::type timestamp;

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

                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

               namespace forget
               {
                  struct Request : basic_message< Type::queue_dequeue_forget_request>
                  {
                     common::process::Handle process;
                     std::size_t queue = 0;
                     std::string name;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & queue;
                        archive & name;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  struct Reply : basic_message< Type::queue_dequeue_forget_reply>
                  {
                     bool found = false;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & found;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // forget


            } // dequeue


            struct Queue
            {
               enum class Type : int
               {
                  group_error_queue = 1,
                  error_queue = 2,
                  queue = 3,
               };

               using id_type = std::size_t;

               Queue() = default;
               Queue( std::string name, std::size_t retries) : name( std::move( name)), retries( retries) {}
               Queue( std::string name) : Queue( std::move( name), 0) {};

               id_type id = 0;
               std::string name;
               std::size_t retries = 0;
               id_type error = 0;
               Type type = Type::queue;



               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & id;
                  archive & name;
                  archive & retries;
                  archive & error;
                  archive & type;
               })

               friend std::ostream& operator << ( std::ostream& out, const Type& value);
               friend std::ostream& operator << ( std::ostream& out, const Queue& value);
            };
            static_assert( traits::is_movable< Queue>::value, "not movable");




            namespace information
            {

               struct Queue : message::queue::Queue
               {
                  std::size_t count;
                  std::size_t size;
                  std::size_t uncommitted;
                  platform::time::point::type timestamp;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     message::queue::Queue::marshal( archive);
                     archive & count;
                     archive & size;
                     archive & uncommitted;
                     archive & timestamp;
                  })
               };
               static_assert( traits::is_movable< Queue>::value, "not movable");

               template< message::Type type>
               struct basic_information : basic_message< type>
               {

                  common::process::Handle process;
                  std::vector< Queue> queues;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_message< type>::marshal( archive);
                     archive & process;
                     archive & queues;
                  })

               };

               struct Message : queue::base_message_information
               {
                  std::size_t queue;
                  std::size_t origin;
                  platform::binary::type trid;
                  std::size_t state;
                  std::size_t redelivered;
                  platform::time::point::type timestamp;

                  std::size_t size;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     queue::base_message_information::marshal( archive);
                     archive & queue;
                     archive & origin;
                     archive & trid;
                     archive & state;
                     archive & timestamp;
                     archive & size;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Message& value);
               };
               static_assert( traits::is_movable< Message>::value, "not movable");


               namespace queues
               {

                  using Request = server::basic_id< Type::queue_queues_information_request>;

                  using Reply = basic_information< Type::queue_queues_information_reply>;

               } // queues

               namespace messages
               {
                  struct Request : server::basic_id< Type::queue_queue_information_request>
                  {
                     using base_type = server::basic_id< Type::queue_queue_information_request>;

                     Queue::id_type qid = 0;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & qid;
                     })

                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  struct Reply : common::message::server::basic_id< Type::queue_queue_information_reply>
                  {
                     using base_type = common::message::server::basic_id< Type::queue_queue_information_reply>;

                     std::vector< Message> messages;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & messages;
                     })

                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // messages

            } // information

            using Information = information::basic_information< Type::queue_information>;


            namespace peek
            {
               namespace information
               {
                  struct Request : basic_message< Type::queue_peek_information_request>
                  {
                     common::process::Handle process;
                     std::size_t queue = 0;
                     std::string name;
                     dequeue::Selector selector;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & queue;
                        archive & name;
                        archive & selector;
                     })
                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };

                  struct Reply : basic_message< Type::queue_peek_information_reply>
                  {
                     std::vector< queue::information::Message> messages;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & messages;
                     })
                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };

               } // information

               namespace messages
               {
                  struct Request : basic_message< Type::queue_peek_messages_request>
                  {
                     common::process::Handle process;
                     std::vector< Uuid> ids;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & ids;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");


                  struct Reply : basic_message< Type::queue_peek_messages_reply>
                  {
                     std::vector< dequeue::Reply::Message> messages;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & messages;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");


               } // messages

            } // peek




            namespace connect
            {
               struct Request : basic_message< Type::queue_connect_request>
               {
                  common::process::Handle process;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & process;
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_message< Type::queue_connect_reply>
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
               static_assert( traits::is_movable< Reply>::value, "not movable");
            } // connect


            namespace restore
            {

               struct Request : basic_message< Type::queue_restore_request>
               {
                  common::process::Handle process;
                  std::vector< Queue::id_type> queues;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & process;
                     archive & queues;
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_message< Type::queue_restore_request>
               {
                  struct Affected
                  {
                     Queue::id_type queue;
                     std::size_t restored = 0;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        archive & queue;
                        archive & restored;
                     })
                  };

                  std::vector< Affected> affected;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & affected;
                  })
               };
            } // restore



         } // queue

         namespace reverse
         {



            template<>
            struct type_traits< queue::enqueue::Request> : detail::type< queue::enqueue::Reply> {};
            template<>
            struct type_traits< queue::dequeue::Request> : detail::type< queue::dequeue::Reply> {};

            template<>
            struct type_traits< queue::lookup::Request> : detail::type< queue::lookup::Reply> {};


            template<>
            struct type_traits< queue::peek::information::Request> : detail::type< queue::peek::information::Reply> {};

            template<>
            struct type_traits< queue::peek::messages::Request> : detail::type< queue::peek::messages::Reply> {};

            template<>
            struct type_traits< queue::restore::Request> : detail::type< queue::restore::Reply> {};

         } // reverse


      } // message
   } // common



} // casual

#endif // QUEUE_H_
