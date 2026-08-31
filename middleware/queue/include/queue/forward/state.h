//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "queue/common/ipc/message.h"
#include "queue/common/ipc.h"

#include "casual/assert.h"

#include "common/serialize/macro.h"
#include "common/strong/type.h"
#include "common/strong/id.h"
#include "common/process.h"
#include "common/buffer/type.h"
#include "common/state/machine.h"
#include "common/communication/select.h"
#include "common/communication/ipc/send.h"

#include <vector>
#include <string>
#include <iosfwd>
#include <optional>

namespace casual
{
   namespace queue::forward
   {
      namespace state
      {
         enum class Runlevel : short
         {
            startup,
            running,
            shutdown,
         };
         std::string_view description( Runlevel value);

         namespace forward
         {
            namespace detail
            {
               struct policy
               {
                  inline static int generate() 
                  {
                     static int value{};
                     return ++value;
                  }
               };
            } // detail

            using id = common::strong::Type< int, detail::policy>;

            struct Source
            {
               std::string queue;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( queue);
               )
            };

            namespace queue
            {
               struct Target 
               {
                  std::string queue;
                  common::chronology::duration delay{};

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( delay);
                  )
               };
            } // queue

            struct Instances
            {
               platform::size::type configured{};
               platform::size::type running{};
               bool enabled = true;

               //! increment and decrement running instances
               Instances& operator++();
               Instances& operator--();

               //! @returns how many concurrent that are missing before we
               //! reach the configured...
               inline platform::size::type missing() const 
               { 
                  if( ! enabled || configured < running)
                     return 0;
                  return configured - running;
               }

               inline platform::size::type surplus() const
               {
                  if( ! enabled)
                     return running;

                  if( configured > running)
                     return 0;
                  return running - configured;
               }

               //! @returns true if there are no configured and running currently
               inline auto absent() const { return configured == 0 && running == 0;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( configured);
                  CASUAL_SERIALIZE( running);
                  CASUAL_SERIALIZE( enabled);
               )
            };

            struct Metric
            {
               struct Count
               {
                  platform::size::type count = 0;
                  common::chronology::time_point last{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( count);
                     CASUAL_SERIALIZE( last);
                  )
               };

               Count commit;
               Count rollback;

               inline auto transactions() const { return commit.count + rollback.count;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( commit);
                  CASUAL_SERIALIZE( rollback);
               )
            };

            struct Service
            {
               struct Target
               {
                  std::string service;
                  
                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( service);
                  )
               };

               using Reply = forward::queue::Target;

               forward::id id = forward::id::generate();

               forward::Source source;
               Target target;
               std::optional< Reply> reply;
               forward::Instances instances;
               Metric metric;
               ipc::message::group::dequeue::Selector selector;

               std::string alias;
               std::string note;


               inline friend bool operator == ( const Service& lhs, forward::id rhs) { return lhs.id == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( source);
                  CASUAL_SERIALIZE( target);
                  CASUAL_SERIALIZE( reply);
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( metric);
                  CASUAL_SERIALIZE( selector);
               )
            };


            struct Queue
            {
               forward::id id = forward::id::generate();

               forward::Source source;
               forward::queue::Target target;
               forward::Instances instances;
               Metric metric;
               ipc::message::group::dequeue::Selector selector;

               std::string alias;
               std::string note;

               inline friend bool operator == ( const Queue& lhs, forward::id rhs) { return lhs.id == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( source);
                  CASUAL_SERIALIZE( target);
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( metric);
                  CASUAL_SERIALIZE( selector);
               )
            };

         } // forward

         namespace pending
         {
            struct base
            {
               forward::id id;
               common::strong::correlation::id correlation;

               inline friend bool operator == ( const base& lhs, forward::id rhs) { return lhs.id == rhs;}
               inline friend bool operator == ( const base& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( correlation);
               )
            };

            struct transaction_base : base
            {
               common::transaction::ID trid;

               CASUAL_LOG_SERIALIZE(
                  base::serialize( archive);
                  CASUAL_SERIALIZE( trid);
               )
            };

            namespace queue
            {
               namespace source
               {
                  struct Lookup : base
                  {
                  };
                  
               } // source
               
               namespace target
               {
                  struct Lookup : transaction_base
                  {
                     common::buffer::Payload buffer;

                     //! possible delay for the upcoming enqueued message. 
                     common::chronology::duration delay{};

                     CASUAL_LOG_SERIALIZE(
                        transaction_base::serialize( archive);
                        CASUAL_SERIALIZE( buffer);
                        CASUAL_SERIALIZE( delay);
                     )
                  };

                  namespace lookup
                  {
                     struct Discard : transaction_base
                     {
                     };
                  }
                  
               } // target

            } // queue

            struct Dequeue : transaction_base
            {
               //! if we need to cancel the blocking dequeue.
               common::strong::ipc::id ipc;

               CASUAL_LOG_SERIALIZE(
                  transaction_base::serialize( archive);
                  CASUAL_SERIALIZE( ipc);
               )
            };

            namespace service
            {
               struct Lookup : transaction_base
               {
                  common::buffer::Payload payload;

                  CASUAL_LOG_SERIALIZE(
                     transaction_base::serialize( archive);
                     CASUAL_SERIALIZE( payload);
                  )
               };

               namespace lookup
               {
                  struct Discard : transaction_base
                  {
                  };
               }

               struct Call : transaction_base
               {
                  common::process::Handle target{};

                  inline friend bool operator == ( const Call& lhs, common::process::compare_equal_to_handle auto rhs) { return lhs.target == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     transaction_base::serialize( archive);
                     CASUAL_SERIALIZE( target);
                  )
               };

            } // service

            namespace transaction
            {
               struct Rollback : transaction_base
               {
               };

               struct Commit : transaction_base
               {
               };
            } // transaction

            struct Enqueue : transaction_base
            {
            };

         } // pending

         //! This is the actual 'state machine' of the forwards.
         //! requests are sent and we keep a pending correlation for
         //! that request.
         //! When the reply arrives we get invoked with the exact context (handle::???)
         //! (where we are in the statemachine for the specific forward),
         //! and we consume/remove the current pending, make the next request
         //! and add that specific 'correlation' to pending.
         //! 
         //! The layout of Pending has the order of normal successful execution flow,
         //! with the exception of pending.rollbacks, which is a 'state shortcut' when
         //! handlers detect some errors and rollbacks the 'flow'.
         //! Otherwise the flow ends when we handle commit reply, and start the Forward
         //! blocking dequeue again.    
         struct Pending
         {
            struct
            {
               struct
               {
                  //! these are blocking lookups, and the start of the 'flow' for a forward.
                  std::vector< state::pending::queue::source::Lookup> source;
                  //! blocking lookups for the target queues.
                  std::vector< state::pending::queue::target::Lookup> target;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( source);
                     CASUAL_SERIALIZE( target);
                  )

               } lookup;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( lookup);
               )

            } queue;

            std::vector< state::pending::Dequeue> dequeues;


            struct
            {
               std::vector< state::pending::service::Lookup> lookups;
               std::vector< state::pending::service::Call> calls;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( lookups);
                  CASUAL_SERIALIZE( calls);
               )

            } service;

            std::vector< state::pending::Enqueue> enqueues;

            struct
            {
               std::vector< state::pending::transaction::Commit> commits;
               std::vector< state::pending::transaction::Rollback> rollbacks;
               

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( commits);
                  CASUAL_SERIALIZE( rollbacks);
               )

            } transaction;

            struct
            {
               struct
               {
                  struct
                  {
                     std::vector< state::pending::queue::source::Lookup> source;
                     std::vector< state::pending::queue::target::lookup::Discard> target;

                     CASUAL_LOG_SERIALIZE(
                        CASUAL_SERIALIZE( source);
                        CASUAL_SERIALIZE( target);
                     )

                  } queue;

                  std::vector< state::pending::service::lookup::Discard> service;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( service);
                  )

               } lookup;

               std::vector< state::pending::Dequeue> dequeues;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( lookup);
                  CASUAL_SERIALIZE( dequeues);
               )

            } discard;
            
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( queue);
               CASUAL_SERIALIZE( dequeues);
               CASUAL_SERIALIZE( service);
               CASUAL_SERIALIZE( enqueues);
               CASUAL_SERIALIZE( transaction);
               CASUAL_SERIALIZE( discard);
            )
         };

         struct Forward
         {
            std::vector< state::forward::Service> services;
            std::vector< state::forward::Queue> queues;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( queues);
            )
         };

      } // state

      struct State
      {  
         common::state::Machine< state::Runlevel> runlevel;

         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         state::Forward forward;
         state::Pending pending;

         std::string alias;

         std::vector< std::string> memberships;

         //! we're done when we're in shutdown mode
         //! and all forwards has no concurrent stuff in flight.
         bool done() const noexcept;

         //! removes all state associated with source and/or target for forwards 
         void invalidate( const std::vector< state::forward::id>& ids) noexcept;

         state::forward::Service* forward_service( state::forward::id id) noexcept;
         state::forward::Queue* forward_queue( state::forward::id id) noexcept;

         //! applies functor to found forward, either service-forward or queue-forward
         template< typename F>
         auto forward_apply( state::forward::id id, F&& functor)
         {
            if( auto found = forward_service( id))
               return functor( *found);

            return functor( *assertion( forward_queue( id), "failed to find id: ", id));
         }

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( forward);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( memberships);
         )
      };
   } // queue::forward
} // casual