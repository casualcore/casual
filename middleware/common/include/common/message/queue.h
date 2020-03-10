//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/type.h"
#include "casual/platform.h"
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

            namespace message
            {
               enum class State : int
               {
                  enqueued = 1,
                  committed = 2,
                  dequeued = 3,
               };
            } // message

            struct base_message_information
            {
               common::Uuid id;

               std::string properties;
               std::string reply;

               platform::time::point::type available;
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
               platform::binary::type payload;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  base_message_information::serialize( archive);
                  CASUAL_SERIALIZE( payload);
               })
            };
            static_assert( traits::is_movable< base_message>::value, "not movable");

            namespace lookup
            {
               using base_request = basic_request< Type::queue_lookup_request>;
               struct Request : base_request
               {
                   using base_request::base_request;

                  std::string name;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( name);
                  })
               };

               using base_reply = basic_reply< Type::queue_lookup_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  strong::queue::id queue;
                  size_type order = 0;

                  explicit operator bool () const { return ! process.ipc.empty();}

                  CASUAL_CONST_CORRECT_SERIALIZE({
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( order);
                  })

                  bool local() const;
               };

            } // lookup

            namespace enqueue
            {
               using base_request = basic_request< Type::queue_enqueue_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  using Message = base_message;

                  common::transaction::ID trid;
                  strong::queue::id queue;
                  std::string name;

                  Message message;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( message);
                  })
               };

               using base_reply = basic_message< Type::queue_enqueue_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  common::Uuid id;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_reply::serialize( archive);
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

               using base_request = basic_request< Type::queue_dequeue_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  common::transaction::ID trid;
                  strong::queue::id queue;
                  std::string name;
                  Selector selector;
                  bool block = false;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( selector);
                     CASUAL_SERIALIZE( block);
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               using base_reply = basic_message< Type::queue_dequeue_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  struct Message : base_message
                  {
                     Message( const base_message& m) : base_message( m) {}
                     Message() = default;

                     size_type redelivered = 0;
                     platform::time::point::type timestamp;

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
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( message);
                  })
               };

               namespace forget
               {
                  using base_request = basic_request< Type::queue_dequeue_forget_request>;
                  struct Request : base_request
                  {
                     using base_request::base_request;

                     strong::queue::id queue;
                     std::string name;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( queue);
                        CASUAL_SERIALIZE( name);
                     })
                  };

                  using base_reply = basic_message< Type::queue_dequeue_forget_reply>;
                  struct Reply : base_reply
                  {
                     bool found = false;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( found);
                     })
                  };

               } // forget
            } // dequeue

            struct Queue
            {
               enum class Type : int
               {
                  queue = 1,
                  error_queue = 2,
               };

               struct Retry 
               {
                  size_type count = 0;
                  platform::time::unit delay{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( count);
                     CASUAL_SERIALIZE( delay);
                  })
               };


               Queue() = default;
               inline Queue( std::string name, Retry retry) : name{ std::move( name)}, retry{ retry} {}
               inline Queue( std::string name) : name{ std::move( name)} {};

               strong::queue::id id;
               std::string name;
               Retry retry;
               strong::queue::id error;
               inline Type type() const { return  error ? Type::queue : Type::error_queue;}

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( retry);
                  CASUAL_SERIALIZE( error);
               })

               inline friend bool operator == ( const Queue& lhs, strong::queue::id id) { return lhs.id == id;}
               inline friend bool operator == ( const Queue& lhs, const std::string& name) { return lhs.name == name;}

               inline friend std::ostream& operator << ( std::ostream& out, Type value)
               {
                  switch( value)
                  {
                     case Type::error_queue: return out << "error_queue";
                     case Type::queue: return out << "queue";
                  }
                  return out << "unknown";
               }
            };


            namespace information
            {
               using base_queue = common::message::queue::Queue;
               struct Queue : base_queue
               {
                  size_type count = 0;
                  size_type size = 0;
                  size_type uncommitted = 0;
                  platform::time::point::type timestamp;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_queue::serialize( archive);
                     CASUAL_SERIALIZE( count);
                     CASUAL_SERIALIZE( size);
                     CASUAL_SERIALIZE( uncommitted);
                     CASUAL_SERIALIZE( timestamp);
                  })
               };

               template< common::message::Type type>
               struct basic_information : basic_request< type>
               {
                  using basic_request< type>::basic_request;
                  
                  std::string name;
                  std::vector< Queue> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     basic_request< type>::serialize( archive);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( queues);
                  })

               };

               struct Message : queue::base_message_information
               {
                  strong::queue::id queue;
                  strong::queue::id origin;
                  platform::binary::type trid;
                  message::State state;
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

               namespace queues
               {
                  using Request = common::message::basic_request< Type::queue_queues_information_request>;
                  using Reply = basic_information< Type::queue_queues_information_reply>;

               } // queues

               namespace messages
               {
                  using base_request = common::message::basic_request< Type::queue_queue_information_request>;
                  struct Request : base_request
                  {
                     using base_request::base_request;

                     strong::queue::id qid;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( qid);
                     })

                  };
                  
                  using base_reply = common::message::basic_reply< Type::queue_queue_information_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;

                     std::vector< Message> messages;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( messages);
                     })

                  };

               } // messages

            } // information

            using Information = information::basic_information< Type::queue_information>;


            namespace peek
            {
               namespace information
               {
                  using base_request = basic_request< Type::queue_peek_information_request>;
                  struct Request : base_request
                  {
                     using base_request::basic_request;

                     strong::queue::id queue;
                     std::string name;
                     dequeue::Selector selector;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( queue);
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( selector);
                     })
                  };

                  using base_reply = basic_message< Type::queue_peek_information_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;

                     std::vector< queue::information::Message> messages;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( messages);
                     })
                  };

               } // information

               namespace messages
               {
                  using base_request = basic_request< Type::queue_peek_messages_request>;
                  struct Request : base_request
                  {
                     using base_request::base_request;

                     std::vector< Uuid> ids;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( ids);
                     })
                  };

                  using base_reply = basic_message< Type::queue_peek_messages_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;

                     std::vector< dequeue::Reply::Message> messages;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( messages);
                     })
                  };

               } // messages
            } // peek


            namespace connect
            {
               using Request = basic_request< Type::queue_connect_request>;

               using base_reply = basic_message< Type::queue_connect_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  std::string name;
                  std::vector< Queue> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( queues);
                  })
               };
            } // connect

            namespace concurrent
            {
               namespace advertise
               {
                  struct Queue
                  {
                     Queue() = default;

                     inline Queue( std::string name, size_type retries) : name{ std::move( name)}, retries{ retries} {}
                     inline Queue( std::string name) : name{ std::move( name)} {}

                     std::string name;
                     size_type retries = 0;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( retries);
                     })
                  };
               } // advertise
               
               using base_advertise = basic_request< Type::queue_advertise>;
               struct Advertise : base_advertise
               {
                  using base_advertise::base_advertise;

                  platform::size::type order = 0;

                  struct
                  {
                     std::vector< advertise::Queue> add;
                     std::vector< std::string> remove;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( add);
                        CASUAL_SERIALIZE( remove);
                     })
                  } queues;

                  //! indicate to remove all current advertised queues, and replace with content in this message
                  bool reset = false;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_advertise::serialize( archive);
                     CASUAL_SERIALIZE( order);
                     CASUAL_SERIALIZE( queues);
                     CASUAL_SERIALIZE( reset);
                  })
               };

            } // concurrent

            struct Affected
            {
               strong::queue::id queue;
               size_type count = 0;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( count);
               })
            };

            namespace restore
            {
               using base_request = basic_request< Type::queue_restore_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  std::vector< strong::queue::id> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( queues);
                  })
               };

               using base_reply = basic_message< Type::queue_restore_request>;
               struct Reply : base_reply
               {
                  std::vector< Affected> affected;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( affected);
                  })
               };
               
            } // restore

            namespace clear
            {

               using base_request = basic_request< Type::queue_clear_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  std::vector< strong::queue::id> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( queues);
                  })
               };


               using base_reply = basic_message< Type::queue_restore_request>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  std::vector< Affected> affected;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( affected);
                  })

               };
            } // clear

            namespace messages
            {
               namespace remove
               {
                  using base_request = basic_request< Type::queue_messages_remove_request>;
                  struct Request : base_request
                  {
                     using base_request::base_request;

                     strong::queue::id queue;
                     //! messages to remove
                     std::vector< Uuid> ids;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( queue);
                        CASUAL_SERIALIZE( ids);
                     })
                  };

                  using base_reply = basic_message< Type::queue_messages_remove_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;
                     //! messages that got removed
                     std::vector< Uuid> ids;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( ids);
                     })
                  };
                  
               } // remove
            } // messages
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
            struct type_traits< queue::dequeue::forget::Request> : detail::type< queue::dequeue::forget::Reply> {};


            template<>
            struct type_traits< queue::peek::information::Request> : detail::type< queue::peek::information::Reply> {};

            template<>
            struct type_traits< queue::peek::messages::Request> : detail::type< queue::peek::messages::Reply> {};

            template<>
            struct type_traits< queue::restore::Request> : detail::type< queue::restore::Reply> {};

            template<>
            struct type_traits< queue::clear::Request> : detail::type< queue::clear::Reply> {};

            template<>
            struct type_traits< queue::messages::remove::Request> : detail::type< queue::messages::remove::Reply> {};

            template<>
            struct type_traits< queue::information::messages::Request> : detail::type< queue::information::messages::Reply> {};



            template<>
            struct type_traits< queue::connect::Request> : detail::type< queue::connect::Reply> {};
            

         } // reverse


      } // message
   } // common



} // casual


