//!
//! test_server_database.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "queue/group/group.h"

#include "common/transaction/id.h"



namespace casual
{
   namespace queue
   {
      namespace local
      {
         namespace
         {
            common::message::queue::dequeue::Request create_request( std::size_t queue, bool block = true)
            {
               common::message::queue::dequeue::Request result;

               result.queue = queue;
               result.block = block;

               return result;
            }
         } // <unnamed>
      } // local
      TEST( casual_queue_group_pending, empty_commit__expect_no_pending)
      {
         auto trid = common::transaction::ID::create();

         group::State::Pending pending;

         auto result = pending.commit( trid);

         EXPECT_TRUE( result.enqueued.empty());
         EXPECT_TRUE( result.requests.empty());
      }


      TEST( casual_queue_group_pending, block_request_q10__enqueue_q10__commit__expect_1_pending)
      {
         auto trid = common::transaction::ID::create();

         group::State::Pending pending;

         pending.dequeue( local::create_request( 10));
         pending.enqueue( trid, 10);

         auto result = pending.commit( trid);

         ASSERT_TRUE( result.requests.size() == 1);
         EXPECT_TRUE( result.requests.at( 0).block);
         EXPECT_TRUE( result.requests.at( 0).queue == 10);
         ASSERT_TRUE( result.enqueued.size() == 1);
         EXPECT_TRUE( result.enqueued.at( 10) == 1);
      }

      TEST( casual_queue_group_pending, block_request_q10__enqueue_q20__commit__expect_0_pending)
      {
         auto trid = common::transaction::ID::create();

         group::State::Pending pending;

         pending.dequeue( local::create_request( 10));
         pending.enqueue( trid, 20);

         auto result = pending.commit( trid);

         EXPECT_TRUE( result.enqueued.empty());
         EXPECT_TRUE( result.requests.empty());
      }

      TEST( casual_queue_group_pending, block_request_q10__enqueue_3x_q10__commit__expect_1_pending)
      {
         auto trid = common::transaction::ID::create();

         group::State::Pending pending;

         pending.dequeue( local::create_request( 10));
         pending.enqueue( trid, 10);
         pending.enqueue( trid, 10);
         pending.enqueue( trid, 10);

         auto result = pending.commit( trid);

         ASSERT_TRUE( result.requests.size() == 1);
         EXPECT_TRUE( result.requests.at( 0).block);
         EXPECT_TRUE( result.requests.at( 0).queue == 10);
         ASSERT_TRUE( result.enqueued.size() == 1);
         EXPECT_TRUE( result.enqueued.at( 10) == 3);
      }


      TEST( casual_queue_group_pending, one_request_q10__enqueue_q10__commit__expect_0_pending)
      {
         auto trid = common::transaction::ID::create();

         group::State::Pending pending;

         pending.dequeue( local::create_request( 10, false));
         pending.enqueue( trid, 10);

         auto result = pending.commit( trid);

         EXPECT_TRUE( result.enqueued.empty());
         EXPECT_TRUE( result.requests.empty());
      }


      TEST( casual_queue_group_pending, block_request_2x_q10__enqueue_q10__commit__expect_2_pending)
      {
         auto trid = common::transaction::ID::create();

         group::State::Pending pending;

         pending.dequeue( local::create_request( 10));
         pending.dequeue( local::create_request( 10));

         pending.enqueue( trid, 10);

         auto result = pending.commit( trid);

         ASSERT_TRUE( result.requests.size() == 2);
         EXPECT_TRUE( result.requests.at( 0).block);
         EXPECT_TRUE( result.requests.at( 0).queue == 10);
         EXPECT_TRUE( result.requests.at( 1).block);
         EXPECT_TRUE( result.requests.at( 1).queue == 10);
         ASSERT_TRUE( result.enqueued.size() == 1);
         EXPECT_TRUE( result.enqueued.at( 10) == 1);
      }

      TEST( casual_queue_group_pending, block_request_q10_q20___enqueue_q10__commit__expect_1_pending)
      {
         auto trid = common::transaction::ID::create();

         group::State::Pending pending;

         pending.dequeue( local::create_request( 10));
         pending.dequeue( local::create_request( 20));

         pending.enqueue( trid, 10);

         auto result = pending.commit( trid);

         ASSERT_TRUE( result.requests.size() == 1);
         EXPECT_TRUE( result.requests.at( 0).block);
         EXPECT_TRUE( result.requests.at( 0).queue == 10);
         ASSERT_TRUE( result.enqueued.size() == 1);
         EXPECT_TRUE( result.enqueued.at( 10) == 1);
      }

      TEST( casual_queue_group_pending, block_request_q10_q20___enqueue_q10_g20__commit__expect_2_pending)
      {
         auto trid = common::transaction::ID::create();

         group::State::Pending pending;

         pending.dequeue( local::create_request( 10));
         pending.dequeue( local::create_request( 20));

         pending.enqueue( trid, 10);
         pending.enqueue( trid, 20);

         auto result = pending.commit( trid);

         ASSERT_TRUE( result.requests.size() == 2);
         EXPECT_TRUE( result.requests.at( 0).block);
         EXPECT_TRUE( result.requests.at( 0).queue == 10);
         EXPECT_TRUE( result.requests.at( 1).block);
         EXPECT_TRUE( result.requests.at( 1).queue == 20);
         ASSERT_TRUE( result.enqueued.size() == 2);
         EXPECT_TRUE( result.enqueued.at( 10) == 1);
         EXPECT_TRUE( result.enqueued.at( 20) == 1);
      }

   } // queue
} // casual
