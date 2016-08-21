//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "queue/broker/state.h"

namespace casual
{
   namespace queue
   {


      TEST( casual_queue_broker_state, correlation__empty__expect_empty_stage)
      {
         common::unittest::Trace trace;

         broker::State::Correlation correlation{ common::process::handle(), common::Uuid{}, {}};

         EXPECT_TRUE( correlation.replied());
         EXPECT_TRUE( correlation.stage() == broker::State::Correlation::Stage::empty);
      }

      TEST( casual_queue_broker_state, correlation__1_request_pending___expect_pending_stage)
      {
         common::unittest::Trace trace;

         broker::State::Correlation correlation{ common::process::handle(), common::Uuid{}, { common::process::Handle{ 10, 10}}};

         EXPECT_TRUE( ! correlation.replied());
         EXPECT_TRUE( correlation.stage() == broker::State::Correlation::Stage::pending);
      }

      TEST( casual_queue_broker_state, correlation__1_request_replied___expect_replied_stage)
      {
         common::unittest::Trace trace;

         common::process::Handle group_1{ 10, 10};

         broker::State::Correlation correlation{ common::process::handle(), common::Uuid{}, { group_1}};
         correlation.stage( group_1, broker::State::Correlation::Stage::replied);

         EXPECT_TRUE( correlation.replied());
         EXPECT_TRUE( correlation.stage() == broker::State::Correlation::Stage::replied);
      }

      TEST( casual_queue_broker_state, correlation__2_request_pending___expect_pending_stage)
      {
         common::unittest::Trace trace;

         common::process::Handle group_1{ 10, 10};
         common::process::Handle group_2{ 20, 20};

         broker::State::Correlation correlation{ common::process::handle(), common::Uuid{}, { group_1, group_2}};

         EXPECT_TRUE( ! correlation.replied());
         EXPECT_TRUE( correlation.stage() == broker::State::Correlation::Stage::pending);
      }

      TEST( casual_queue_broker_state, correlation__2_request_pending_replied___expect_pending_stage)
      {
         common::unittest::Trace trace;

         common::process::Handle group_1{ 10, 10};
         common::process::Handle group_2{ 20, 20};

         broker::State::Correlation correlation{ common::process::handle(), common::Uuid{}, { group_1, group_2}};

         correlation.stage( group_2, broker::State::Correlation::Stage::replied);

         EXPECT_TRUE( ! correlation.replied());
         EXPECT_TRUE( correlation.stage() == broker::State::Correlation::Stage::pending);
      }

      TEST( casual_queue_broker_state, correlation__2_request_pending_error___expect_pending_stage)
      {
         common::unittest::Trace trace;

         common::process::Handle group_1{ 10, 10};
         common::process::Handle group_2{ 20, 20};

         broker::State::Correlation correlation{ common::process::handle(), common::Uuid{}, { group_1, group_2}};

         correlation.stage( group_2, broker::State::Correlation::Stage::error);

         EXPECT_TRUE( ! correlation.replied());
         EXPECT_TRUE( correlation.stage() == broker::State::Correlation::Stage::pending);
      }

      TEST( casual_queue_broker_state, correlation__2_request_replied___expect_replied_stage)
      {
         common::unittest::Trace trace;

         common::process::Handle group_1{ 10, 10};
         common::process::Handle group_2{ 20, 20};

         broker::State::Correlation correlation{ common::process::handle(), common::Uuid{}, { group_1, group_2}};

         correlation.stage( group_1, broker::State::Correlation::Stage::replied);
         correlation.stage( group_2, broker::State::Correlation::Stage::replied);

         EXPECT_TRUE( correlation.replied());
         EXPECT_TRUE( correlation.stage() == broker::State::Correlation::Stage::replied);
      }

      TEST( casual_queue_broker_state, correlation__2_request_error_replied___expect_replied_error_stage)
      {
         common::unittest::Trace trace;

         common::process::Handle group_1{ 10, 10};
         common::process::Handle group_2{ 20, 20};

         broker::State::Correlation correlation{ common::process::handle(), common::Uuid{}, { group_1, group_2}};

         correlation.stage( group_1, broker::State::Correlation::Stage::replied);
         correlation.stage( group_2, broker::State::Correlation::Stage::error);

         EXPECT_TRUE( correlation.replied());
         EXPECT_TRUE( correlation.stage() == broker::State::Correlation::Stage::error);
      }


   } // queue

} // casual
