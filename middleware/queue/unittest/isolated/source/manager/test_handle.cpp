//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "queue/manager/manager.h"
#include "queue/manager/handle.h"


#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"

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

               State()
               {
                  state.groups = {
                        { "group10", group10.process()},
                        { "group20", group20.process()},
                  };

                  state.queues = {
                        { "queue1", { { group10.process(), 1}}},
                        { "queue2", { { group10.process(), 2}}},
                        { "queue3", { { group10.process(), 3}}},
                        { "queueB1", { { group20.process(), 1}}},
                        { "queueB2", { { group20.process(), 2}}},
                        { "queueB3", { { group20.process(), 3}}},
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
                  manager::handle::domain::Advertise{ state.state},
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
               catch( const common::exception::Shutdown&)
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

            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::id(), request);
            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::id(), common::message::shutdown::Request{});
         }

         local::handle( state);

         {
            common::message::queue::lookup::Reply reply;
            common::communication::ipc::blocking::receive( requester.output(), reply);

            EXPECT_TRUE( reply.queue == 1);
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

            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::id(), request);
            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::id(), common::message::shutdown::Request{});
         }

         local::handle( state);

         {
            common::message::queue::lookup::Reply reply;
            common::communication::ipc::blocking::receive( requester.output(), reply);

            EXPECT_TRUE( reply.queue == 0);
            EXPECT_TRUE( reply.process == common::process::Handle{}) << "reply.process: " << reply.process;
         }
      }

      TEST( casual_queue_manager_handle, domain_advertise_queueX__expect_added_queueX)
      {
         common::unittest::Trace trace;

         local::State state;

         common::mockup::ipc::Collector requester;

         common::domain::Identity remote{ "remote-domain"};

         {
            //
            // Send Advertise
            //
            common::message::gateway::domain::Advertise advertise;
            advertise.process = requester.process();
            advertise.domain = remote;
            advertise.order = 1;
            advertise.queues = { { "queueX" }};;

            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::id(), advertise);
            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::id(), common::message::shutdown::Request{});
         }

         local::handle( state);

         {
            auto& instances = state.state.queues.at( "queueX");

            ASSERT_TRUE( instances.size() == 1);
            EXPECT_TRUE( instances.front().process == requester.process()) << "reply.process: " << instances.front().process;
            // expect order to be pumped one
            EXPECT_TRUE( instances.front().order == 2) << " instances.front().order: " <<  instances.front().order;


            auto found = common::range::find_if( state.state.gateways, [&]( const manager::State::Gateway& g){
               return g.process == requester.process();
            });

            ASSERT_TRUE( ! found.empty());
            EXPECT_TRUE( found->id == remote);
         }
      }


      TEST( casual_queue_manager_handle, pending_for_queueX__domain_advertise_queueX__expect_lookup_reply)
      {
         common::unittest::Trace trace;

         local::State state;


         common::mockup::ipc::Collector requester;
         common::domain::Identity remote{ "remote-domain"};


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
            common::message::gateway::domain::Advertise advertise;
            advertise.process = requester.process();
            advertise.domain = remote;
            advertise.order = 1;
            advertise.queues = { { "queueX" }};;

            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::id(), advertise);
            common::mockup::ipc::eventually::send( common::communication::ipc::inbound::id(), common::message::shutdown::Request{});
         }

         local::handle( state);

         {
            common::message::queue::lookup::Reply reply;
            common::communication::ipc::blocking::receive( requester.output(), reply);

            EXPECT_TRUE( reply.queue == 0);
            EXPECT_TRUE( reply.process == requester.process()) << "reply.process: " << reply.process;
         }
      }


   } // queue
} // casual
