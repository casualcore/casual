//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/message/type.h"
#include "common/transaction/id.h"
#include "common/transaction/global.h"
#include "common/buffer/type.h"
#include "common/code/queue.h"

#include "configuration/model.h"

#include <optional>

namespace casual
{
   namespace queue::ipc::message
   {
      struct Retry 
      {
         platform::size::type count{};
         platform::time::unit delay{};

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( count);
            CASUAL_SERIALIZE( delay);
         )
      };

      namespace advertise
      {
         namespace queue
         {
            using Retry = message::Retry;

            struct Enable
            {
               bool enqueue = true;
               bool dequeue = true;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( enqueue);
                  CASUAL_SERIALIZE( dequeue);
               )
            };
            
         } // queue

         struct Queue
         {
            std::string name;
            queue::Retry retry;
            queue::Enable enable;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( retry);
               CASUAL_SERIALIZE( enable);
            )
         };

         enum struct Directive
         {
            update,   //! regular update
            reset,    //! remove all previous associated queues for the 'device'
            instance, //! instance (order) only 
         };

         constexpr std::string_view description( Directive value) noexcept
         {
            switch( value)
            {
               case Directive::update: return "update";
               case Directive::reset: return "reset";
               case Directive::instance: return "instance";
            }
            return "<unknown>";
         }

      } // advertise

      using base_advertise = common::message::basic_request< common::message::Type::queue_manager_queue_advertise>;
      struct Advertise : base_advertise
      {
         using base_advertise::base_advertise;
         
         // the alias of the process that advertised
         std::string alias;
         //! A human readable description of the "instance"
         std::string description;
         platform::size::type order = 0;
         advertise::Directive directive{};

         struct
         {
            std::vector< advertise::Queue> add;
            std::vector< std::string> remove;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( add);
               CASUAL_SERIALIZE( remove);
            )
         } queues;

         CASUAL_CONST_CORRECT_SERIALIZE(
            base_advertise::serialize( archive);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( description);
            CASUAL_SERIALIZE( order);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( queues);
         )
      };

      namespace lookup
      {
         namespace request
         {
            namespace context
            {
               enum class Semantic : short
               {
                  direct,
                  wait
               };

               inline constexpr std::string_view description( Semantic value) noexcept
               {
                  switch( value)
                  {
                     case Semantic::direct: return "direct";
                     case Semantic::wait: return "wait";
                  }
                  return "unknown";
               }

               enum class Requester : short
               {
                  internal,
                  external,
                  // connection configured with discovery.forward 
                  external_discovery,
               };
               
               inline constexpr std::string_view description( Requester value) noexcept
               {
                  switch( value)
                  {
                     case Requester::internal: return "internal";
                     case Requester::external: return "external"; 
                     case Requester::external_discovery: return "external_discovery";                     
                  }
                  return "<unknown>";
               }

               enum class Action : short
               {
                  any,
                  enqueue,
                  dequeue,
               };

               inline constexpr std::string_view description( Action value) noexcept
               {
                  switch( value)
                  {
                     case Action::any: return "any";
                     case Action::enqueue: return "enqueue";
                     case Action::dequeue: return "dequeue";
                  }
                  return "<unknown>";
               }

            } // context

            struct Context
            {
               context::Semantic semantic = context::Semantic::direct;
               context::Requester requester = context::Requester::internal;
               context::Action action = context::Action::any; 

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( semantic);
                  CASUAL_SERIALIZE( requester);
                  CASUAL_SERIALIZE( action);
               )
            };

         } // request

         using base_request = common::message::basic_request< common::message::Type::queue_manager_queue_lookup_request>;
         struct Request : base_request
         {
            using base_request::base_request;

            request::Context context;
            std::string name;
            common::transaction::global::ID gtrid;

            inline friend bool operator == ( const Request& lhs, const std::string& name) { return lhs.name == name;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( context);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( gtrid);
            )
         };

         using base_reply = common::message::basic_process< common::message::Type::queue_manager_queue_lookup_reply>;
         struct Reply : base_reply
         {
            using base_reply::base_reply;

            common::strong::queue::id queue;
            std::string name;
            platform::size::type order{};

            inline explicit operator bool () const { return process.ipc.valid();}
            inline bool remote() const { return order > 0;}

            CASUAL_CONST_CORRECT_SERIALIZE({
               base_reply::serialize( archive);
               CASUAL_SERIALIZE( queue);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( order);
            })
         };

         namespace discard
         {
            using base_request = common::message::basic_request< common::message::Type::queue_manager_queue_lookup_discard_request>;

            //! only correlation is used to correlate previous lookup requests.
            using Request = base_request;

            namespace reply
            {
               enum class State : short
               {
                  replied,
                  discarded,
               };

               inline constexpr std::string_view description( State value) noexcept
               {
                  switch( value)
                  {
                     case State::discarded: return "discarded";
                     case State::replied: return "replied";
                  }
                  return "unknown";
               }
            } // reply

            using base_reply = common::message::basic_message< common::message::Type::queue_manager_queue_lookup_discard_reply>;
            struct Reply : base_reply
            {
               reply::State state = reply::State::replied;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( state);
               )
            };
         } // discard
      } // lookup

      struct Attributes : common::compare::Equality< Attributes>
      {
         std::string properties;
         std::string reply;
         platform::time::point::type available;

         inline auto tie() const noexcept { return std::tie( properties, reply, available);}
         
         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( properties);
            CASUAL_SERIALIZE( reply);
            CASUAL_SERIALIZE( available);
         ) 
      };

      using Payload = common::buffer::Payload;

      namespace group
      {
         namespace queue
         {
            namespace affected
            {
               struct Queue
               {
                  common::strong::queue::id id{};
                  std::string name;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( name);
                  )
               };   
            } // affected

            struct Affected
            {
               affected::Queue queue;
               platform::size::type count{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( count);
               )
            };

            namespace restore
            {
               using base_request = common::message::basic_request< common::message::Type::queue_group_queue_restore_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  std::vector< common::strong::queue::id> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( queues);
                  )
               };

               using base_reply = common::message::basic_message< common::message::Type::queue_group_queue_restore_reply>;
               struct Reply : base_reply
               {
                  std::vector< Affected> affected;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( affected);
                  )
               };
               
            } // restore

            namespace clear
            {
               using base_request = common::message::basic_request< common::message::Type::queue_group_queue_clear_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  std::vector< common::strong::queue::id> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( queues);
                  )
               };

               using base_reply = common::message::basic_message< common::message::Type::queue_group_queue_clear_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  std::vector< Affected> affected;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( affected);
                  )

               };
            } // clear
         } // queue

         namespace metric
         {
            namespace reset
            {
               using base_request = common::message::basic_request< common::message::Type::queue_group_metric_reset_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  std::vector< common::strong::queue::id> queues;
                  
                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( queues);
                  )
               };

               using Reply = common::message::basic_reply< common::message::Type::queue_group_metric_reset_reply>;
               
            } // reset
            
         } // metric

         namespace dequeue
         {
            struct Selector
            {
               std::string properties;
               common::Uuid id;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( properties);
                  CASUAL_SERIALIZE( id);
               )
            };

            using base_request = common::message::basic_request< common::message::Type::queue_group_dequeue_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               common::transaction::ID trid;
               common::strong::queue::id queue;
               std::string name;
               Selector selector;
               bool block = false;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( selector);
                  CASUAL_SERIALIZE( block);
               )
            };

            struct Message
            {
               common::Uuid id;
               ipc::message::Attributes attributes;
               ipc::message::Payload payload;
               platform::size::type redelivered{};
               platform::time::point::type timestamp;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( attributes);
                  CASUAL_SERIALIZE( payload);
                  CASUAL_SERIALIZE( redelivered);
                  CASUAL_SERIALIZE( timestamp);
               )
            };

            using base_reply = common::message::basic_message< common::message::Type::queue_group_dequeue_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               std::optional< dequeue::Message> message;
               common::code::queue code{};

               inline explicit operator bool () const noexcept { return message.has_value();}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( message);
                  CASUAL_SERIALIZE( code);
               )
            };

            namespace v1_2
            {
               using base_reply = common::message::basic_message< common::message::Type::queue_group_dequeue_reply_v1_2>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  std::vector< dequeue::Message> message;

                  inline explicit operator bool () const noexcept { return ! message.empty();}

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( message);
                  )
               };
            } // v1_2

            namespace forget
            {
               using base_request = common::message::basic_request< common::message::Type::queue_group_dequeue_forget_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  common::strong::queue::id queue;
                  std::string name;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( name);
                  )
               };

               using base_reply = common::message::basic_message< common::message::Type::queue_group_dequeue_forget_reply>;
               struct Reply : base_reply
               {
                  //! true if callee actually discarded/forgot about the
                  //! pending dequeue.
                  //! false if callee don't know abut a pending dequeue, hence already
                  //! got a dequeue.
                  bool discarded = false;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( discarded);
                  )
               };

            } // forget
         } // dequeue

         namespace enqueue
         {
            struct Message
            {
               Message() = default;
               Message( dequeue::Message other)
                  : id{ other.id}, attributes{ std::move( other.attributes)},
                     payload{ std::move( other.payload)}
               {}

               common::Uuid id;
               ipc::message::Attributes attributes;
               ipc::message::Payload payload;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( attributes);
                  CASUAL_SERIALIZE( payload);
               )
            };

            using base_request = common::message::basic_request< common::message::Type::queue_group_enqueue_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               common::transaction::ID trid;
               common::strong::queue::id queue;
               std::string name;
               enqueue::Message message;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( message);
               )
            };

            using base_reply = common::message::basic_message< common::message::Type::queue_group_enqueue_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               common::Uuid id;
               common::code::queue code{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( code);
               )
            };

            namespace v1_2
            {
               using base_reply = common::message::basic_message< common::message::Type::queue_group_enqueue_reply_v1_2>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  //! A 'nil' id represent that the enqueue failed (for now, assume queue:no_queue)
                  common::Uuid id;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( id);
                  )
               };
            }

         } // enqueue

         using Connect = common::message::basic_request< common::message::Type::queue_group_connect>;

         namespace configuration::update
         {
            using base_request = common::message::basic_request< common::message::Type::queue_group_configuration_update_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               casual::configuration::model::queue::Group model;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( model);
               )
            };

            struct Queue 
            {
               common::strong::queue::id id;
               std::string name;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( name);
               )
            };

            using base_reply = common::message::basic_process< common::message::Type::queue_group_configuration_update_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               std::string alias;
               std::vector< Queue> queues;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( queues);
               )
            };
            
         } // configuration::update

         namespace state
         {
            using Request = common::message::basic_request< common::message::Type::queue_group_state_request>;

            namespace queue
            {
               enum class Type : int
               {
                  queue = 1,
                  error_queue = 2,
               };
               inline constexpr std::string_view description( Type type) noexcept
               {
                  switch( type)
                  {
                     case Type::queue: return "queue";
                     case Type::error_queue: return "error_queue";
                  }
                  return "<unknown>";
               }

               struct Metric
               {
                  platform::size::type count{};
                  platform::size::type size{};
                  platform::size::type uncommitted{};
                  platform::time::point::type last;
                  platform::size::type dequeued{};
                  platform::size::type enqueued{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( count);
                     CASUAL_SERIALIZE( size);
                     CASUAL_SERIALIZE( uncommitted);
                     CASUAL_SERIALIZE( dequeued);
                     CASUAL_SERIALIZE( enqueued);
                     CASUAL_SERIALIZE( last);
                  )
               };

            } // queue

            struct Queue 
            {
               common::strong::queue::id id;
               std::string name;
               Retry retry;
               common::strong::queue::id error;
               queue::Metric metric;
               platform::time::point::type created;

               inline queue::Type type() const { return  error ? queue::Type::queue : queue::Type::error_queue;}

               inline friend bool operator == ( const Queue& lhs, const std::string& rhs) { return lhs.name == rhs;}
               inline friend bool operator == ( const Queue& lhs, common::strong::queue::id rhs) { return lhs.id == rhs;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( retry);
                  CASUAL_SERIALIZE( error);
                  CASUAL_SERIALIZE( metric);
                  CASUAL_SERIALIZE( created);
               )
            };

            namespace reply
            {
               struct Size
               {
                  platform::size::type current;
                  std::optional< platform::size::type> capacity;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( current);
                     CASUAL_SERIALIZE( capacity);
                  )
               };
            }

            using base_reply = common::message::basic_request< common::message::Type::queue_group_state_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               std::string alias;
               std::string queuebase;
               std::string note;

               std::vector< Queue> queues;
               std::vector< common::strong::queue::id> zombies;

               reply::Size size;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( queuebase);
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( queues);
                  CASUAL_SERIALIZE( zombies);
                  CASUAL_SERIALIZE( size);
               )
            };

         } // state

         namespace message
         {
            enum class State : int
            {
               enqueued = 1,
               committed = 2,
               dequeued = 3,
            };
            inline constexpr std::string_view description( State state) noexcept
            {
               switch( state)
               {
                  case State::enqueued: return "enqueued";
                  case State::committed: return "committed";
                  case State::dequeued: return "dequeued";
               }
               return "<unknown>";
            }


            struct Meta
            {
               common::Uuid id;
               common::strong::queue::id queue;
               common::strong::queue::id origin;
               std::string properties;
               std::string reply;
               message::State state;
               platform::binary::type trid;
               platform::time::point::type available;
               platform::size::type redelivered;
               std::string type;
               platform::size::type size;
               platform::time::point::type timestamp;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( origin);
                  CASUAL_SERIALIZE( properties);
                  CASUAL_SERIALIZE( reply);
                  CASUAL_SERIALIZE( state);
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( available);
                  CASUAL_SERIALIZE( redelivered);
                  CASUAL_SERIALIZE( type);
                  CASUAL_SERIALIZE( size);
                  CASUAL_SERIALIZE( timestamp);
               )
            };

            namespace meta
            {
               using base_request = common::message::basic_request< common::message::Type::queue_group_message_meta_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  common::strong::queue::id qid;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( qid);
                  )
               };

               using base_reply = common::message::basic_reply< common::message::Type::queue_group_message_meta_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  std::vector< message::Meta> messages;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( messages);
                  )
               };

               //! 'meta' peek, to get meta info from _selector_
               namespace peek
               {
                  using base_request = common::message::basic_request< common::message::Type::queue_group_message_meta_peek_request>;
                  struct Request : base_request
                  {
                     common::strong::queue::id queue;
                     std::string name;
                     dequeue::Selector selector;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( queue);
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( selector);
                     )
                  };

                  using base_reply = common::message::basic_reply< common::message::Type::queue_group_message_meta_peek_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;

                     std::vector< message::Meta> messages;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( messages);
                     )
                  };
               } // peek
               
            } // meta

            //! ordinary peek
            namespace peek
            {
               using base_request = common::message::basic_request< common::message::Type::queue_group_message_peek_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  std::vector< common::Uuid> ids;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( ids);
                  )
               };

               using base_reply = common::message::basic_message< common::message::Type::queue_group_message_peek_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  std::vector< dequeue::Message> messages;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( messages);
                  )
               };

            } // peek

            namespace browse
            {
               using base_request = common::message::basic_request< common::message::Type::queue_group_message_browse_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  common::strong::queue::id queue;

                  //! Should be set to the last browsed message timestamp (or nothing if it's the first request),
                  //! hence act as the pivot point of which messages to browse.
                  platform::time::point::type last{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( last);
                  )
               };

               using base_reply = common::message::basic_message< common::message::Type::queue_group_message_browse_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  std::optional< dequeue::Message> message;

                  inline explicit operator bool() const noexcept { return common::predicate::boolean( message);}

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( message);
                  )
               };
               
            } // browse

            namespace remove
            {
               using base_request = common::message::basic_request< common::message::Type::queue_group_message_remove_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  common::strong::queue::id queue;
                  //! messages to remove
                  std::vector< common::Uuid> ids;
                  bool force = false;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( ids);
                     CASUAL_SERIALIZE( force);
                  )
               };

               using base_reply = common::message::basic_message< common::message::Type::queue_group_message_remove_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;
                  //! messages that got removed
                  std::vector< common::Uuid> ids;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( ids);
                  )
               };
               
            } // remove
            namespace recovery
            {
               enum class Directive : int
               {
                  commit,
                  rollback
               };

               inline constexpr std::string_view description( Directive value) noexcept
               {
                  switch( value)
                  {
                     using Enum = recovery::Directive;
                     case Enum::commit: return "commit";
                     case Enum::rollback: return "rollback";
                  }
                  return "<unknown>";
               }


               using base_request = common::message::basic_request< common::message::Type::queue_group_message_recovery_request>;
               struct Request : base_request
               {

                  using base_request::base_request;

                  common::strong::queue::id queue;
                  //! transactions to recover
                  std::vector< common::transaction::global::ID> gtrids;

                  Directive directive = Directive::commit;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( gtrids);
                     CASUAL_SERIALIZE( directive);
                  )
               };

               using base_reply = common::message::basic_message< common::message::Type::queue_group_message_recovery_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;
                  //! transaction that got handled
                  std::vector< common::transaction::global::ID> gtrids;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( gtrids);
                  )
               };
            } // recovery
         } // message
      } // group

      namespace forward::group
      {
         using Connect = common::message::basic_request< common::message::Type::queue_forward_group_connect>;

         namespace configuration::update
         {
            using base_request = common::message::basic_request< common::message::Type::queue_forward_group_configuration_update_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               casual::configuration::model::queue::forward::Group model;
               std::vector< casual::configuration::model::domain::Group> groups;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( model);
                  CASUAL_SERIALIZE( groups);
               )
            };

            using base_reply = common::message::basic_process< common::message::Type::queue_forward_group_configuration_update_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               std::string alias;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( alias);
               )
            };
            
         } // configuration::update


         namespace state
         {
            using Request = common::message::basic_request< common::message::Type::queue_forward_group_state_request>;

            using base_reply = common::message::basic_request< common::message::Type::queue_forward_group_state_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               struct Instances
               {
                  platform::size::type configured = 0;
                  platform::size::type running = 0;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( configured);
                     CASUAL_SERIALIZE( running);
                  )
               };

               struct Metric
               {
                  struct Count
                  {
                     platform::size::type count = 0;
                     platform::time::point::type last{};

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( count);
                        CASUAL_SERIALIZE( last);
                     )
                  };

                  Count commit;
                  Count rollback;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( commit);
                     CASUAL_SERIALIZE( rollback);
                  )
               };

               struct Queue
               {
                  struct Target
                  {
                     std::string queue;
                     platform::time::unit delay{};

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( queue);
                        CASUAL_SERIALIZE( delay);
                     )
                  };

                  std::string alias;
                  std::string source;
                  Target target;
                  Instances instances;
                  Metric metric;
                  std::string note;
                  bool enabled = true;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( alias);
                     CASUAL_SERIALIZE( source);
                     CASUAL_SERIALIZE( target);
                     CASUAL_SERIALIZE( instances);
                     CASUAL_SERIALIZE( metric);
                     CASUAL_SERIALIZE( note);
                     CASUAL_SERIALIZE( enabled);
                  )
               };

               struct Service
               {
                  struct Target
                  {
                     std::string service;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( service);
                     )
                  };

                  using Reply = Queue::Target;
                  
                  std::string alias;
                  std::string source;
                  Target target;
                  Instances instances;
                  std::optional< Reply> reply;
                  Metric metric;
                  std::string note;
                  bool enabled = true;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( alias);
                     CASUAL_SERIALIZE( source);
                     CASUAL_SERIALIZE( target);
                     CASUAL_SERIALIZE( instances);
                     CASUAL_SERIALIZE( reply);
                     CASUAL_SERIALIZE( metric);
                     CASUAL_SERIALIZE( note);
                     CASUAL_SERIALIZE( enabled);
                  )
               };

               std::string alias;
               std::vector< Service> services;
               std::vector< Queue> queues;
               std::string note;
               bool enabled = true;

               inline friend Reply operator + ( Reply lhs, Reply rhs)
               {
                  common::algorithm::move( rhs.services, std::back_inserter( lhs.services));
                  common::algorithm::move( rhs.queues, std::back_inserter( lhs.queues));

                  return lhs;
               }

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( enabled);
               )
            };

         } // state 

      } // forward::group

   } // queue::ipc::message
   
   namespace common::message::reverse
   {
      template<>
      struct type_traits< casual::queue::ipc::message::lookup::Request> : detail::type< casual::queue::ipc::message::lookup::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::lookup::discard::Request> : detail::type< casual::queue::ipc::message::lookup::discard::Reply> {};


      template<>
      struct type_traits< casual::queue::ipc::message::group::configuration::update::Request> : detail::type< casual::queue::ipc::message::group::configuration::update::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::state::Request> : detail::type< casual::queue::ipc::message::group::state::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::message::meta::Request> : detail::type< casual::queue::ipc::message::group::message::meta::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::message::meta::peek::Request> : detail::type< casual::queue::ipc::message::group::message::meta::peek::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::message::peek::Request> : detail::type< casual::queue::ipc::message::group::message::peek::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::message::browse::Request> : detail::type< casual::queue::ipc::message::group::message::browse::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::message::remove::Request> : detail::type< casual::queue::ipc::message::group::message::remove::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::message::recovery::Request> : detail::type< casual::queue::ipc::message::group::message::recovery::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::enqueue::Request> : detail::type< casual::queue::ipc::message::group::enqueue::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::dequeue::Request> : detail::type< casual::queue::ipc::message::group::dequeue::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::dequeue::forget::Request> : detail::type< casual::queue::ipc::message::group::dequeue::forget::Reply> {};


      template<>
      struct type_traits< casual::queue::ipc::message::group::queue::restore::Request> : detail::type< casual::queue::ipc::message::group::queue::restore::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::queue::clear::Request> : detail::type< casual::queue::ipc::message::group::queue::clear::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::group::metric::reset::Request> : detail::type< casual::queue::ipc::message::group::metric::reset::Reply> {};


      template<>
      struct type_traits< casual::queue::ipc::message::forward::group::configuration::update::Request> : detail::type< casual::queue::ipc::message::forward::group::configuration::update::Reply> {};

      template<>
      struct type_traits< casual::queue::ipc::message::forward::group::state::Request> : detail::type< casual::queue::ipc::message::forward::group::state::Reply> {};
   
   } // common::message::reverse
} // casual