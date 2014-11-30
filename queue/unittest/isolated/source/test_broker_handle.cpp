//!
//! test_broker_handle.cpp
//!
//! Created on: Oct 12, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "queue/broker/broker.h"
#include "queue/broker/handle.h"


#include "common/mockup/ipc.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"

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
                     { "group10", group10.server()},
                     { "group20", group20.server()},
               };

               state.queues = {
                     { "queue1", { group10.server(), 1}},
                     { "queue2", { group10.server(), 2}},
                     { "queue3", { group10.server(), 3}},
                     { "queueB1", { group20.server(), 1}},
                     { "queueB2", { group20.server(), 2}},
                     { "queueB3", { group20.server(), 3}},
               };
            }

            common::mockup::ipc::Instance group10;
            common::mockup::ipc::Instance group20;

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
               broker::handle::peek::queue::Request{ state.state},
               common::message::handle::Shutdown{},
            };
         }

         void handle( State& state)
         {
            auto handler = test::handler( state);

            common::queue::blocking::Reader read( common::ipc::receive::queue());

            try
            {
               while( true)
               {
                  handler.dispatch( read.next());
               }
            }
            catch( const common::exception::Shutdown&)
            {
               // we're done
            }

         }

      } // test

      TEST( casual_queue_broker, handle_lookup_request)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::ipc::receive::id()};
         common::mockup::ipc::Instance requester{ 42};

         common::queue::blocking::Send send;

         {
            //
            // Send reqeust
            //
            common::message::queue::lookup::Request request;
            request.process = requester.server();
            request.name = "queue1";
            send( broker.id(), request);


            send( broker.id(), common::message::shutdown::Request{});
         }

         test::handle( state);

         {
            common::queue::blocking::Reader read( requester.receive());
            common::message::queue::lookup::Reply reply;

            read( reply);

            EXPECT_TRUE( reply.queue == 1);
            EXPECT_TRUE( reply.process == state.group10.server());

         }
      }

      TEST( casual_queue_broker, handle_lookup_request_absent_queue__expect_0_as_queue)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::ipc::receive::id()};
         common::mockup::ipc::Instance requester{ 42};

         common::queue::blocking::Send send;

         {
            //
            // Send reqeust
            //
            common::message::queue::lookup::Request request;
            request.process = requester.server();
            request.name = "absent_qeueue";
            send( broker.id(), request);


            send( broker.id(), common::message::shutdown::Request{});
         }

         test::handle( state);

         {
            common::queue::blocking::Reader read( requester.receive());
            common::message::queue::lookup::Reply reply;

            read( reply);

            EXPECT_TRUE( reply.queue == 0);
            EXPECT_TRUE( reply.process == common::process::Handle{}) << "reply.process: " << reply.process;
         }
      }

      TEST( casual_queue_broker, handle_group_involved)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::ipc::receive::id()};

         common::queue::blocking::Send send;

         auto trid = common::transaction::ID::create();

         {
            //
            // Send reqeust
            //
            common::message::queue::group::Involved request;
            request.process = state.group10.server();
            request.trid = trid;
            send( broker.id(), request);


            send( broker.id(), common::message::shutdown::Request{});
         }

         test::handle( state);

         ASSERT_TRUE( state.state.involved.at( trid).size() == 1);
         EXPECT_TRUE( state.state.involved.at( trid).at( 0) == state.group10.server());
      }

      TEST( casual_queue_broker, handle_group_involved__xid1_1_group__xid2_2_groups)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::ipc::receive::id()};

         common::queue::blocking::Send send;

         auto trid1 = common::transaction::ID::create();
         auto trid2 = common::transaction::ID::create();

         {
            //
            // Send request
            //
            common::message::queue::group::Involved request;
            request.process = state.group10.server();
            request.trid = trid1;
            send( broker.id(), request);

            request.process = state.group20.server();
            request.trid = trid2;
            send( broker.id(), request);

            request.process = state.group10.server();
            request.trid = trid2;
            send( broker.id(), request);


            send( broker.id(), common::message::shutdown::Request{});
         }

         test::handle( state);

         ASSERT_TRUE( state.state.involved.at( trid1).size() == 1);
         EXPECT_TRUE( state.state.involved.at( trid1).at( 0) == state.group10.server());

         ASSERT_TRUE( state.state.involved.at( trid2).size() == 2);
         EXPECT_TRUE( state.state.involved.at( trid2).at( 0) == state.group20.server());
         EXPECT_TRUE( state.state.involved.at( trid2).at( 1) == state.group10.server());
      }


      TEST( casual_queue_broker, handle_commit_request)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::ipc::receive::id()};
         common::mockup::ipc::Instance requester{ 42};

         common::queue::blocking::Send send;

         auto trid = common::transaction::ID::create();

         {
            // prep state
            state.state.involved[ trid].push_back( state.group10.server());
         }

         {
            //
            // Send reqeust
            //
            common::message::transaction::resource::commit::Request request;
            request.process = requester.server();
            request.trid = trid;
            request.resource = 42;
            request.flags = 10;
            send( broker.id(), request);


            send( broker.id(), common::message::shutdown::Request{});
         }

         test::handle( state);

         {
            // group 10 should get the request, since it's involved with the xid
            common::queue::blocking::Reader read( state.group10.receive());
            common::message::transaction::resource::commit::Request request;

            read( request);

            EXPECT_TRUE( request.process.queue == common::ipc::receive::id());
            EXPECT_TRUE( request.trid == trid);
            EXPECT_TRUE( request.resource == 42) << "request.resource: " << request.resource;
            EXPECT_TRUE( request.flags == 10);

            // group10 should not be involved
            EXPECT_TRUE( state.state.involved.count( trid) == 0);
            // Should be a waiting correlation for the xid
            EXPECT_TRUE( state.state.correlation.at( trid).caller == requester.server());
            EXPECT_TRUE( state.state.correlation.at( trid).requests.at( 0).group == state.group10.server());
            EXPECT_TRUE( state.state.correlation.at( trid).requests.at( 0).state == broker::State::Correlation::State::pending);
         }
      }

      TEST( casual_queue_broker, handle_commit_request__2_groups_involed__expect_reqeust_to_both)
      {
         test::State state;

         common::mockup::ipc::Router broker{ common::ipc::receive::id()};
         common::mockup::ipc::Instance requester{ 42};

         common::queue::blocking::Send send;

         auto trid = common::transaction::ID::create();

         {
            // prep state
            state.state.involved[ trid].push_back( state.group10.server());
            state.state.involved[ trid].push_back( state.group20.server());
         }

         {
            //
            // Send reqeust
            //
            common::message::transaction::resource::commit::Request request;
            request.process = requester.server();
            request.trid = trid;
            request.resource = 42;
            request.flags = 10;
            send( broker.id(), request);


            send( broker.id(), common::message::shutdown::Request{});
         }

         test::handle( state);

         auto check = [&]( common::mockup::ipc::Instance& group, int index)
         {
            // group 10 should get the request, since it's involved with the xid
            common::queue::blocking::Reader read( group.receive());
            common::message::transaction::resource::commit::Request request;

            read( request);

            EXPECT_TRUE( request.process.queue == common::ipc::receive::id());
            EXPECT_TRUE( request.trid == trid);
            EXPECT_TRUE( request.resource == 42);
            EXPECT_TRUE( request.flags == 10);

            // group should not be involved
            EXPECT_TRUE( state.state.involved.count( trid) == 0);
            // Should be a waiting correlation for the xid
            EXPECT_TRUE( state.state.correlation.at( trid).caller == requester.server());
            EXPECT_TRUE( state.state.correlation.at( trid).requests.at( index).group == group.server());
            EXPECT_TRUE( state.state.correlation.at( trid).requests.at( index).state == broker::State::Correlation::State::pending);
         };

         check( state.group10, 0);
         check( state.group20, 1);
      }


   } // queue


} // casual
