//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "queue/broker/broker.h"
#include "queue/broker/handle.h"


#include "common/mockup/ipc.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/trace.h"

namespace casual
{
   namespace queue
   {
      namespace test
      {
         struct State
         {

            State() : group10( 10), group20( 20)
            {
               state.groups = {
                     { "group10", group10.process()},
                     { "group20", group20.process()},
               };

               state.queues = {
                     { "queue1", { group10.process(), 1}},
                     { "queue2", { group10.process(), 2}},
                     { "queue3", { group10.process(), 3}},
                     { "queueB1", { group20.process(), 1}},
                     { "queueB2", { group20.process(), 2}},
                     { "queueB3", { group20.process(), 3}},
               };
            }

            common::mockup::ipc::Collector group10;
            common::mockup::ipc::Collector group20;

            //common::mockup::ipc::Router router;

            broker::State state;

         };

         common::message::dispatch::Handler handler( State& state)
         {
            return common::message::dispatch::Handler{
               broker::handle::connect::Request{ state.state},
               broker::handle::lookup::Request{ state.state},
               broker::handle::group::Involved{ state.state},
               broker::handle::transaction::commit::Request{ state.state},
               broker::handle::transaction::commit::Reply{ state.state},
               broker::handle::transaction::rollback::Request{ state.state},
               broker::handle::transaction::rollback::Reply{ state.state},
               //broker::handle::peek::queue::Request{ state.state},
               common::message::handle::Shutdown{},
            };
         }

         void handle( State& state)
         {
            auto handler = test::handler( state);

            try
            {
               while( true)
               {
                  handler( common::communication::ipc::inbound::device().next(
                        common::communication::ipc::policy::Blocking{}));
               }
            }
            catch( const common::exception::Shutdown&)
            {
               // we're done
            }

         }

      } // test

      /*
      TEST( casual_queue_broker, handle_lookup_request)
      {
         common::Trace trace{ "TEST casual_queue_broker, handle_lookup_request" };
         test::State state;

         common::mockup::ipc::Router broker{ common::communication::ipc::inbound::id()};
         common::mockup::ipc::Instance requester{ 42};


         {
            common::communication::ipc::Helper ipc;

            //
            // Send request
            //
            common::message::queue::lookup::Request request;
            request.process = requester.process();
            request.name = "queue1";
            ipc.blocking_send( broker.input(), request);


            ipc.blocking_send( broker.input(), common::message::shutdown::Request{});
         }

         test::handle( state);

         {
            common::message::queue::lookup::Reply reply;
            common::communication::ipc::blocking::receive( requester.output(), reply);

            EXPECT_TRUE( reply.queue == 1);
            EXPECT_TRUE( reply.process == state.group10.process());

         }
      }

      TEST( casual_queue_broker, handle_lookup_request_absent_queue__expect_0_as_queue)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::communication::ipc::inbound::id()};
         common::mockup::ipc::Instance requester{ 42};

         common::communication::ipc::Helper ipc;

         {
            //
            // Send reqeust
            //
            common::message::queue::lookup::Request request;
            request.process = requester.process();
            request.name = "absent_qeueue";
            ipc.blocking_send( broker.input(), request);


            ipc.blocking_send( broker.input(), common::message::shutdown::Request{});
         }

         test::handle( state);

         {
            common::message::queue::lookup::Reply reply;
            common::communication::ipc::blocking::receive( requester.output(), reply);

            EXPECT_TRUE( reply.queue == 0);
            EXPECT_TRUE( reply.process == common::process::Handle{}) << "reply.process: " << reply.process;
         }
      }

      TEST( casual_queue_broker, handle_group_involved)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::communication::ipc::inbound::id()};

         common::communication::ipc::Helper ipc;

         auto trid = common::transaction::ID::create();

         {
            //
            // Send reqeust
            //
            common::message::queue::group::Involved request;
            request.process = state.group10.process();
            request.trid = trid;
            ipc.blocking_send( broker.input(), request);


            ipc.blocking_send( broker.input(), common::message::shutdown::Request{});
         }

         test::handle( state);

         ASSERT_TRUE( state.state.involved.at( trid).size() == 1);
         EXPECT_TRUE( state.state.involved.at( trid).at( 0) == state.group10.process());
      }

      TEST( casual_queue_broker, handle_group_involved__xid1_1_group__xid2_2_groups)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::communication::ipc::inbound::id()};

         common::communication::ipc::Helper ipc;

         auto trid1 = common::transaction::ID::create();
         auto trid2 = common::transaction::ID::create();

         {
            //
            // Send request
            //
            common::message::queue::group::Involved request;
            request.process = state.group10.process();
            request.trid = trid1;
            ipc.blocking_send( broker.input(), request);

            request.process = state.group20.process();
            request.trid = trid2;
            ipc.blocking_send( broker.input(), request);

            request.process = state.group10.process();
            request.trid = trid2;
            ipc.blocking_send( broker.input(), request);


            ipc.blocking_send( broker.input(), common::message::shutdown::Request{});
         }

         test::handle( state);

         ASSERT_TRUE( state.state.involved.at( trid1).size() == 1);
         EXPECT_TRUE( state.state.involved.at( trid1).at( 0) == state.group10.process());

         ASSERT_TRUE( state.state.involved.at( trid2).size() == 2);
         EXPECT_TRUE( state.state.involved.at( trid2).at( 0) == state.group20.process());
         EXPECT_TRUE( state.state.involved.at( trid2).at( 1) == state.group10.process());
      }


      TEST( casual_queue_broker, handle_commit_request)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::communication::ipc::inbound::id()};
         common::mockup::ipc::Instance requester{ 42};

         common::communication::ipc::Helper ipc;

         auto trid = common::transaction::ID::create();

         {
            // prep state
            state.state.involved[ trid].push_back( state.group10.process());
         }

         {
            //
            // Send reqeust
            //
            common::message::transaction::resource::commit::Request request;
            request.process = requester.process();
            request.trid = trid;
            request.resource = 42;
            request.flags = 10;
            ipc.blocking_send( broker.input(), request);


            ipc.blocking_send( broker.input(), common::message::shutdown::Request{});
         }

         test::handle( state);

         {
            // group 10 should get the request, since it's involved with the xid
            common::message::transaction::resource::commit::Request request;
            common::communication::ipc::blocking::receive( state.group10.output(), request);

            EXPECT_TRUE( request.process.queue == common::communication::ipc::inbound::id());
            EXPECT_TRUE( request.trid == trid);
            EXPECT_TRUE( request.resource == 42) << "request.resource: " << request.resource;
            EXPECT_TRUE( request.flags == 10);

            // group10 should not be involved
            EXPECT_TRUE( state.state.involved.count( trid) == 0);
            // Should be a waiting correlation for the xid
            EXPECT_TRUE( state.state.correlation.at( trid).caller == requester.process());
            EXPECT_TRUE( state.state.correlation.at( trid).requests.at( 0).group == state.group10.process());
            EXPECT_TRUE( state.state.correlation.at( trid).requests.at( 0).stage == broker::State::Correlation::Stage::pending);
         }
      }

      TEST( casual_queue_broker, handle_commit_request__2_groups_involed__expect_reqeust_to_both)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::communication::ipc::inbound::id()};
         common::mockup::ipc::Instance requester{ 42};

         common::communication::ipc::Helper ipc;

         auto trid = common::transaction::ID::create();

         {
            // prep state
            state.state.involved[ trid].push_back( state.group10.process());
            state.state.involved[ trid].push_back( state.group20.process());
         }

         {
            //
            // Send reqeust
            //
            common::message::transaction::resource::commit::Request request;
            request.process = requester.process();
            request.trid = trid;
            request.resource = 42;
            request.flags = 10;
            ipc.blocking_send( broker.input(), request);


            ipc.blocking_send( broker.input(), common::message::shutdown::Request{});
         }

         test::handle( state);

         auto check = [&]( common::mockup::ipc::Instance& group, int index)
         {
            // group 10 should get the request, since it's involved with the xid
            common::message::transaction::resource::commit::Request request;
            common::communication::ipc::blocking::receive( group.output(), request);


            EXPECT_TRUE( request.process.queue == common::communication::ipc::inbound::id());
            EXPECT_TRUE( request.trid == trid);
            EXPECT_TRUE( request.resource == 42);
            EXPECT_TRUE( request.flags == 10);

            // group should not be involved
            EXPECT_TRUE( state.state.involved.count( trid) == 0);
            // Should be a waiting correlation for the xid
            EXPECT_TRUE( state.state.correlation.at( trid).caller == requester.process());
            EXPECT_TRUE( state.state.correlation.at( trid).requests.at( index).group == group.process());
            EXPECT_TRUE( state.state.correlation.at( trid).requests.at( index).stage == broker::State::Correlation::Stage::pending);
         };

         check( state.group10, 0);
         check( state.group20, 1);
      }
      */


   } // queue


} // casual
