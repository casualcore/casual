//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "queue/manager/manager.h"
#include "queue/manager/handle.h"


#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/exception/casual.h"

namespace casual
{
   namespace queue
   {
      namespace local
      {
         namespace
         {
            struct State
            {
               template< typename T>
               common::strong::queue::id queue( T id) { return common::strong::queue::id{ id};}

               State()
               {
                  state.groups = {
                        { "group10", group10.process()},
                        { "group20", group20.process()},
                  };

                  state.queues = {
                        { "queue1", { { group10.process(), queue( 1)}}},
                        { "queue2", { { group10.process(), queue( 2)}}},
                        { "queue3", { { group10.process(), queue( 3)}}},
                        { "queueB1", { { group20.process(), queue( 1)}}},
                        { "queueB2", { { group20.process(), queue( 2)}}},
                        { "queueB3", { { group20.process(), queue( 3)}}},
                  };
               }

               common::mockup::domain::Manager manager;

               common::mockup::ipc::Collector group10;
               common::mockup::ipc::Collector group20;

               manager::State state;
            };

            common::communication::ipc::dispatch::Handler handler( State& state)
            {
               return {
                  manager::handle::connect::Request{ state.state},
                  manager::handle::lookup::Request{ state.state},
                  manager::handle::concurrent::Advertise{ state.state},
                  manager::handle::domain::discover::Request{ state.state},
                  //manager::handle::peek::queue::Request{ state.state},
                  common::message::handle::Shutdown{},
               };
            }

            void handle( State& state)
            {
               auto handler = local::handler( state);

               try
               {
                  while( true)
                  {
                     handler( common::communication::ipc::inbound::device().next(
                           common::communication::ipc::policy::Blocking{}));
                  }
               }
               catch( const common::exception::casual::Shutdown&)
               {
                  // we're done
               }

            }

         } // <unnamed>
      } // local


      TEST( casual_queue_manager_handle, lookup_request)
      {
         common::unittest::Trace trace;

         local::State state;

         common::mockup::ipc::Collector requester;


         {
            common::communication::ipc::Helper ipc;

            //
            // Send request
            //
            common::message::queue::lookup::Request request;
            request.process = requester.process();
            request.name = "queue1";

            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::ipc(), request);
            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::ipc(), common::message::shutdown::Request{});
         }

         local::handle( state);

         {
            common::message::queue::lookup::Reply reply;
            common::communication::ipc::blocking::receive( requester.output(), reply);

            EXPECT_TRUE( reply.queue.value() == 1);
            EXPECT_TRUE( reply.process == state.group10.process());

         }
      }



      TEST( casual_queue_manager_handle, lookup_request_absent_queue__expect_0_as_queue)
      {
         common::unittest::Trace trace;



         local::State state;

         common::mockup::ipc::Collector requester;

         {
            //
            // Send reqeust
            //
            common::message::queue::lookup::Request request;
            request.process = requester.process();
            request.name = "absent_qeueue";

            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::ipc(), request);
            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::ipc(), common::message::shutdown::Request{});
         }

         local::handle( state);

         {
            common::message::queue::lookup::Reply reply;
            common::communication::ipc::blocking::receive( requester.output(), reply);

            EXPECT_TRUE( ! reply.queue);
            EXPECT_TRUE( reply.process == common::process::Handle{}); // << "reply.process: " << reply.process;
         }
      }

      TEST( casual_queue_manager_handle, domain_advertise_queueX__expect_added_queueX)
      {
         common::unittest::Trace trace;

         local::State state;

         common::mockup::ipc::Collector requester;

         {
            //
            // Send Advertise
            //
            common::message::queue::concurrent::Advertise advertise;
            advertise.process = requester.process();
            advertise.order = 1;
            advertise.queues = { { "queueX" }};;

            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::ipc(), advertise);
            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::ipc(), common::message::shutdown::Request{});
         }

         local::handle( state);

         {
            auto& instances = state.state.queues.at( "queueX");

            ASSERT_TRUE( instances.size() == 1);
            EXPECT_TRUE( instances.front().process == requester.process()) << "reply.process: " << instances.front().process;
            // expect order to be pumped one
            EXPECT_TRUE( instances.front().order == 2) << " instances.front().order: " <<  instances.front().order;


            auto found = common::algorithm::find_if( state.state.remotes, [&]( const auto& g){
               return g.process == requester.process();
            });

            ASSERT_TRUE( ! found.empty());
            EXPECT_TRUE( found->process == requester.process());
         }
      }


      TEST( casual_queue_manager_handle, pending_for_queueX__domain_advertise_queueX__expect_lookup_reply)
      {
         common::unittest::Trace trace;

         local::State state;


         common::mockup::ipc::Collector requester;


         // prepare request
         {
            common::message::queue::lookup::Request request;

            request.name = "queueX";
            request.process = requester.process();

            state.state.pending.push_back( std::move( request));
         }


         {
            //
            // Send Advertise
            //
            common::message::queue::concurrent::Advertise advertise;
            advertise.process = requester.process();
            advertise.order = 1;
            advertise.queues = { { "queueX" }};;

            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::ipc(), advertise);
            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::ipc(), common::message::shutdown::Request{});
         }

         local::handle( state);

         {
            common::message::queue::lookup::Reply reply;
            common::communication::ipc::blocking::receive( requester.output(), reply);

            EXPECT_TRUE( ! reply.queue);
            EXPECT_TRUE( reply.process == requester.process()) << "reply.process: " << reply.process;
         }
      }


   } // queue
} // casual