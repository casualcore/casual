//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/type.h"
#include "common/platform.h"
#include "common/uuid.h"
#include "common/serialize/macro.h"
#include "common/buffer/type.h"
#include "common/strong/id.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace queue
         {
            using size_type = platform::size::type;

            struct base_message_information
            {
               common::Uuid id;

               std::string properties;
               std::string reply;

               common::platform::time::point::type available;
               std::string type;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( properties);
                  CASUAL_SERIALIZE( reply);
                  CASUAL_SERIALIZE( available);
                  CASUAL_SERIALIZE( type);
               })

            };

            struct base_message : base_message_information
            {
               common::platform::binary::type payload;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  base_message_information::serialize( archive);
                  CASUAL_SERIALIZE( payload);
               })
            };
            static_assert( traits::is_movable< base_message>::value, "not movable");

            namespace lookup
            {
               struct Request : basic_message< Type::queue_lookup_request>
               {
                  common::process::Handle process;
                  std::string name;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( name);
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_message< Type::queue_lookup_reply>
               {
                  Reply() = default;
                  Reply( common::process::Handle process) : process( std::move( process)) {}

                  common::process::Handle process;
                  strong::queue::id queue;
                  size_type order = 0;

                  explicit operator bool () const { return ! process.ipc.empty();}

                  CASUAL_CONST_CORRECT_SERIALIZE({
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( order);
                  })

                  bool local() const;
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
                  strong::queue::id queue;
                  std::string name;

                  Message message;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( message);
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_message< Type::queue_enqueue_reply>
               {
                  common::Uuid id;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( id);
                  })
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

            } // enqueue

            namespace dequeue
            {
               struct Selector
               {
                  std::string properties;
                  common::Uuid id;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( properties);
                     CASUAL_SERIALIZE( id);
                  })
               };

               struct Request : basic_message< Type::queue_dequeue_request>
               {
                  common::process::Handle process;
                  common::transaction::ID trid;
                  strong::queue::id queue;
                  std::string name;
                  Selector selector;
                  bool block = false;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( selector);
                     CASUAL_SERIALIZE( block);
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_message< Type::queue_dequeue_reply>
               {
                  struct Message : base_message
                  {
                     Message( const base_message& m) : base_message( m) {}
                     Message() = default;

                     size_type redelivered = 0;
                     common::platform::time::point::type timestamp;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_message::serialize( archive);

                        CASUAL_SERIALIZE( redelivered);
                        CASUAL_SERIALIZE( timestamp);
                     })
                  };

                  std::vector< Message> message;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( message);
                  })
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

               namespace forget
               {
                  struct Request : basic_message< Type::queue_dequeue_forget_request>
                  {
                     common::process::Handle process;
                     strong::queue::id queue;
                     std::string name;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( process);
                        CASUAL_SERIALIZE( queue);
                        CASUAL_SERIALIZE( name);
                     })
                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  struct Reply : basic_message< Type::queue_dequeue_forget_reply>
                  {
                     bool found = false;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( found);
                     })
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


               Queue() = default;
               Queue( std::string name, size_type retries) : name( std::move( name)), retries( retries) {}
               Queue( std::string name) : Queue( std::move( name), 0) {};

               strong::queue::id id;
               std::string name;
               size_type retries = 0;
               strong::queue::id error;
               Type type = Type::queue;



               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( retries);
                  CASUAL_SERIALIZE( error);
                  CASUAL_SERIALIZE( type);
               })

               inline friend std::ostream& operator << ( std::ostream& out, Type value)
               {
                  switch( value)
                  {
                     case Type::group_error_queue: return out << "group_error_queue";
                     case Type::error_queue: return out << "error_queue";
                     case Type::queue: return out << "queue";
                     default: return out << "unknown";
                  }
               }
            };
            static_assert( traits::is_movable< Queue>::value, "not movable");




            namespace information
            {

               struct Queue : message::queue::Queue
               {
                  size_type count = 0;
                  size_type size = 0;
                  size_type uncommitted = 0;
                  size_type pending = 0;
                  platform::time::point::type timestamp;


                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     message::queue::Queue::serialize( archive);
                     CASUAL_SERIALIZE( count);
                     CASUAL_SERIALIZE( size);
                     CASUAL_SERIALIZE( uncommitted);
                     CASUAL_SERIALIZE( pending);
                     CASUAL_SERIALIZE( timestamp);
                  })
               };
               static_assert( traits::is_movable< Queue>::value, "not movable");

               template< message::Type type>
               struct basic_information : basic_message< type>
               {
                  std::string name;
                  common::process::Handle process;
                  std::vector< Queue> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     basic_message< type>::serialize( archive);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( queues);
                  })

               };

               struct Message : queue::base_message_information
               {
                  strong::queue::id queue;
                  strong::queue::id origin;
                  platform::binary::type trid;
                  size_type state;
                  size_type redelivered;
                  platform::time::point::type timestamp;

                  size_type size;


                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     queue::base_message_information::serialize( archive);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( origin);
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( state);
                     CASUAL_SERIALIZE( timestamp);
                     CASUAL_SERIALIZE( size);
                  })
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

                     strong::queue::id qid;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( qid);
                     })

                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  struct Reply : common::message::server::basic_id< Type::queue_queue_information_reply>
                  {
                     using base_type = common::message::server::basic_id< Type::queue_queue_information_reply>;

                     std::vector< Message> messages;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( messages);
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
                     strong::queue::id queue;
                     std::string name;
                     dequeue::Selector selector;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( process);
                        CASUAL_SERIALIZE( queue);
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( selector);
                     })
                  };

                  struct Reply : basic_message< Type::queue_peek_information_reply>
                  {
                     std::vector< queue::information::Message> messages;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( messages);
                     })
                  };

               } // information

               namespace messages
               {
                  struct Request : basic_message< Type::queue_peek_messages_request>
                  {
                     common::process::Handle process;
                     std::vector< Uuid> ids;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( process);
                        CASUAL_SERIALIZE( ids);
                     })
                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");


                  struct Reply : basic_message< Type::queue_peek_messages_reply>
                  {
                     std::vector< dequeue::Reply::Message> messages;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( messages);
                     })
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // messages
            } // peek




            namespace connect
            {
               struct Request : basic_message< Type::queue_connect_request>
               {
                  common::process::Handle process;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( process);
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_message< Type::queue_connect_reply>
               {
                  std::string name;
                  std::vector< Queue> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( queues);
                  })
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");
            } // connect

            namespace concurrent
            {
               namespace advertise
               {
                  struct Queue
                  {
                     Queue() = default;

                     Queue( std::string name, size_type retries) : name{ std::move( name)}, retries{ retries} {}
                     Queue( std::string name) : name{ std::move( name)} {}

                     std::string name;
                     size_type retries = 0;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( retries);
                     })
                  };
                  static_assert( traits::is_movable< Queue>::value, "not movable");
               } // advertise

               struct Advertise : basic_message< Type::queue_advertise>
               {
                  enum class Directive : short
                  {
                     add,
                     remove,
                     replace
                  };

                  inline friend std::ostream& operator << ( std::ostream& out, Directive value)
                  {
                     switch( value)
                     {
                        case Directive::add: return out << "add";
                        case Directive::remove: return out << "remove";
                        case Directive::replace: return out << "replace";
                        default: return out << "unknown";
                     }
                  }

                  Directive directive = Directive::add;

                  common::process::Handle process;
                  platform::size::type order = 0;
                  std::vector< advertise::Queue> queues;


                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( directive);
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( order);
                     CASUAL_SERIALIZE( queues);
                  })
               };
               static_assert( traits::is_movable< Advertise>::value, "not movable");
            } // concurrent


            namespace restore
            {

               struct Request : basic_message< Type::queue_restore_request>
               {
                  common::process::Handle process;
                  std::vector< strong::queue::id> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( queues);
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_message< Type::queue_restore_request>
               {
                  struct Affected
                  {
                     strong::queue::id queue;
                     size_type restored = 0;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( queue);
                        CASUAL_SERIALIZE( restored);
                     })
                  };

                  std::vector< Affected> affected;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( affected);
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


            template<>
            struct type_traits< queue::connect::Request> : detail::type< queue::connect::Reply> {};

         } // reverse


      } // message
   } // common



} // casual


