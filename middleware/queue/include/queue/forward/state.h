//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/macro.h"
#include "common/value/id.h"
#include "common/strong/id.h"
#include "common/process.h"

#include "common/message/queue.h"


#include <vector>
#include <string>
#include <iosfwd>
#include <optional>

namespace casual
{
   namespace queue
   {
      namespace forward
      {

         namespace state
         {
            enum class Machine : short
            {
               startup,
               running,
               shutdown,
            };

            std::ostream& operator << ( std::ostream& out, Machine value);

            namespace forward
            {
               namespace tag
               {
                  struct type{};
               } // tag
               using id = common::value::basic_id< int, common::value::id::policy::unique_initialize< int, tag::type, 0>>;

               struct Source
               {
                  common::strong::queue::id id;
                  std::string queue;
                  common::process::Handle process;

                  explicit operator bool () const { return id && process;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( process);
                  )
               };

               namespace queue
               {
                  struct Target : Source
                  {
                     platform::time::unit delay{};

                     CASUAL_LOG_SERIALIZE(
                        Source::serialize( archive);
                        CASUAL_SERIALIZE( delay);
                     )
                  };
               } // queue

               struct Instances
               {
                  platform::size::type configured{};
                  platform::size::type running{};

                  //! @returns how many concurrent that hare missing before we
                  //! reach the configured...
                  inline platform::size::type missing() const 
                  { 
                     if( configured < running)
                        return 0;
                     return configured - running;
                  }

                  //! @returns true if there are no configured and running concurrency
                  inline auto absent() const { return configured == 0 && running == 0;}

                  CASUAL_LOG_SERIALIZE(
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

                  forward::id id;

                  forward::Source source;
                  Target target;
                  std::optional< Reply> reply;
                  forward::Instances instances;
                  Metric metric;
                  common::message::queue::dequeue::Selector selector;

                  std::string alias;
                  std::string note;

                  inline bool valid_queues() const { return source && ( ! reply || reply.value());}

                  friend bool operator == ( const Service& lhs, forward::id rhs) { return lhs.id == rhs;}

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
                  forward::id id;

                  forward::Source source;
                  forward::queue::Target target;
                  forward::Instances instances;
                  Metric metric;
                  common::message::queue::dequeue::Selector selector;

                  std::string alias;
                  std::string note;

                  inline bool valid_queues() const { return source && target;}

                  friend bool operator == ( const forward::Queue& lhs, forward::id rhs) { return lhs.id == rhs;}

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
                  common::Uuid correlation;

                  inline friend bool operator == ( const base& lhs, const common::Uuid& rhs) { return lhs.correlation == rhs;}

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
                  struct Lookup : base
                  {

                  };

               } // queue

               struct Dequeue : transaction_base
               {
                  
               };

               namespace service
               {
                  struct Lookup : transaction_base
                  {
                     Lookup() = default;
                     Lookup( transaction_base&& other) : transaction_base{ std::move( other)} {}

                     common::buffer::Payload buffer;

                     CASUAL_LOG_SERIALIZE(
                        transaction_base::serialize( archive);
                        CASUAL_SERIALIZE( buffer);
                     )
                  };

                  struct Call : transaction_base
                  {
                     Call() = default;
                     Call( transaction_base&& other, common::strong::process::id pid)
                        : transaction_base{ std::move( other)}, pid{ pid} {}

                     common::strong::process::id pid{};

                     CASUAL_LOG_SERIALIZE(
                        transaction_base::serialize( archive);
                        CASUAL_SERIALIZE( pid);
                     )
                     
                  };
                  
               } // service

               namespace transaction
               {
                  struct Rollback : transaction_base
                  {
                     Rollback() = default;
                     Rollback( transaction_base&& other) : transaction_base{ std::move( other)} {} 
                  };

                  struct Commit : transaction_base
                  {
                     Commit() = default;
                     Commit( transaction_base&& other) : transaction_base{ std::move( other)} {}
                  };
               } // transaction

               struct Enqueue : transaction_base
               {
                  Enqueue() = default;
                  Enqueue( transaction_base&& other) : transaction_base{ std::move( other)} {}
                  
               };

            } // pending

         } // state


         struct State
         {
            //! This is the actual 'state machine' of the forwards.
            //! requests are sent and we keep a pending correlation for
            //! that request.
            //! When the reply arrives we get invoked with the exact context (handle::???)
            //! (where we are in the statemachine for the specific forward),
            //! and we consume/remove the current pending, make the next request
            //! and add that specific 'correlation' to pending.
            //! 
            //! The layout of Pending has the order of normal successfull execution flow,
            //! with the exception of pending.rollbacks, which is a 'state shortcut' when
            //! handlers detect some errors and rollbacks the 'flow'.
            //! Otherwise the flow ends when we handle commit reply, and start the Forward
            //! blocking dequeue again.    
            struct Pending
            {
               struct
               {
                  std::vector< state::pending::queue::Lookup> lookups;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( lookups);
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
               
               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( dequeues);
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( enqueues);
                  CASUAL_SERIALIZE( transaction);
               )
            };

            struct
            {
               std::vector< state::forward::Service> services;
               std::vector< state::forward::Queue> queues;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
               )

            } forward;

            
            
            Pending pending;
            state::Machine machine = state::Machine::startup;

            //! we're done when we're in shutdown mode
            //! and all forwards has no concurrent stuff in flight.
            bool done() const
            {
               return machine == state::Machine::shutdown &&
                  common::algorithm::all_of( forward.services, []( auto& forward){ return forward.instances.absent();});
            }

            state::forward::Service* forward_service( state::forward::id id)
            {
               if( auto found = common::algorithm::find( forward.services, id))
                  return found.data();
               return nullptr;
            }

            state::forward::Queue* forward_queue( state::forward::id id)
            {
               if( auto found = common::algorithm::find( forward.queues, id))
                  return found.data();
               return nullptr;
            }

            //! applies functor to found forward, either service-forward or queue-forward
            template< typename F>
            auto forward_apply( state::forward::id id, F&& functor)
            {
               if( auto found = forward_service( id))
                  return functor( *found);

               return functor( *forward_queue( id));
            }

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( forward);
               CASUAL_SERIALIZE( machine);
               CASUAL_SERIALIZE( pending);
               
            )
         };


      } // forward
   } // queue
} // casual