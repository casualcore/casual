//!
//! Copyright (c) 2026, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "queue/forward/state.h"

namespace casual
{
   namespace queue::forward
   {
      TEST( casual_queue_forward_state, pending_ctors)
      {
         common::unittest::Trace trace;

         static auto initialize = []( auto pending)
         {
            pending.id = state::forward::id{ 42};
            pending.correlation = common::strong::correlation::id::generate();
            return pending;
         };

         {
            auto source = initialize( state::pending::queue::source::Lookup{});

            auto dequeue = state::pending::Dequeue{ { source}};
            dequeue.trid = common::transaction::id::create();

            auto service_lookup = state::pending::service::Lookup{ dequeue};
            EXPECT_TRUE( service_lookup.id == dequeue.id);
            EXPECT_TRUE( service_lookup.correlation == dequeue.correlation);
            EXPECT_TRUE( service_lookup.trid == dequeue.trid);

            auto service_call = state::pending::service::Call{ service_lookup};
            EXPECT_TRUE( service_call.id == service_lookup.id);
            EXPECT_TRUE( service_call.correlation == service_lookup.correlation);
            EXPECT_TRUE( service_call.trid == service_lookup.trid);

            auto target_lookup = state::pending::queue::target::Lookup{ service_call};
            EXPECT_TRUE( target_lookup.id == service_call.id);
            EXPECT_TRUE( target_lookup.correlation == service_call.correlation);
            EXPECT_TRUE( target_lookup.trid == service_call.trid);

            state::pending::Enqueue enqueue{ target_lookup};
            EXPECT_TRUE( enqueue.id == target_lookup.id);
            EXPECT_TRUE( enqueue.correlation == target_lookup.correlation);
            EXPECT_TRUE( enqueue.trid == target_lookup.trid);

            state::pending::transaction::Commit commit{ enqueue};
            EXPECT_TRUE( commit.id == enqueue.id);
            EXPECT_TRUE( commit.correlation == enqueue.correlation);
            EXPECT_TRUE( commit.trid == enqueue.trid);

            state::pending::transaction::Rollback rollback{ commit};
            EXPECT_TRUE( rollback.id == commit.id);
            EXPECT_TRUE( rollback.correlation == commit.correlation);
            EXPECT_TRUE( rollback.trid == commit.trid);

         }
      }
      
   } // queue::forward
   
} // casual
