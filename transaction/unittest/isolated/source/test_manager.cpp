//!
//! test_manager.cpp
//!
//! Created on: Jan 6, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "transaction/manager/handle.h"


#include "common/mockup/ipc.h"
#include "common/ipc.h"
#include "common/message_dispatch.h"
#include "common/environment.h"




namespace casual
{

   namespace transaction
   {
      namespace local
      {
         namespace ipc
         {
            static common::mockup::Instance< 101> rm1_1;
            static common::mockup::Instance< 102> rm1_2;
            static common::mockup::Instance< 103> rm1_3;

            static common::mockup::Instance< 201> rm2_1;
            static common::mockup::Instance< 202> rm2_2;
            static common::mockup::Instance< 203> rm2_3;


            void clear()
            {
               rm1_1.queue().clear();
               rm1_2.queue().clear();
               rm1_3.queue().clear();
               rm2_1.queue().clear();
               rm2_2.queue().clear();
               rm2_3.queue().clear();

               common::mockup::ipc::broker::queue().clear();
            }

         } // ipc


         State state( state::resource::Proxy::Instance::State mode = state::resource::Proxy::Instance::State::idle)
         {


            State state{ common::environment::directory::temporary() + "/test_transaction_log.db"};

            state::resource::Proxy proxy;

            proxy.id = 1;
            proxy.key = "rm-mockup";
            proxy.concurency = 3;
            proxy.openinfo = "some open info 1";
            proxy.closeinfo = "some close info 1";

            state::resource::Proxy::Instance instance;
            instance.id = 1;
            instance.server.queue_id = ipc::rm1_1.queue().id();
            instance.server.pid = ipc::rm1_1.pid();
            instance.state = mode;

            proxy.instances.push_back( instance);

            instance.server.queue_id = ipc::rm1_2.queue().id();
            instance.server.pid = ipc::rm1_2.pid();
            proxy.instances.push_back( instance);

            instance.server.queue_id = ipc::rm1_3.queue().id();
            instance.server.pid = ipc::rm1_3.pid();
            proxy.instances.push_back( instance);

            state.resources.push_back( proxy);

            proxy.instances.clear();
            proxy.id = 2;
            proxy.openinfo = "some open info 2";
            proxy.closeinfo = "some close info 2";

            instance.id = 2;
            instance.server.queue_id = ipc::rm2_1.queue().id();
            instance.server.pid = ipc::rm2_1.pid();
            proxy.instances.push_back( instance);

            instance.server.queue_id = ipc::rm2_2.queue().id();
            instance.server.pid = ipc::rm2_2.pid();
            proxy.instances.push_back( instance);


            instance.server.queue_id = ipc::rm2_3.queue().id();
            instance.server.pid = ipc::rm2_3.pid();
            proxy.instances.push_back( instance);

            state.resources.push_back( proxy);

            common::range::sort( common::range::make( state.resources));

            return state;
         }



         common::message::dispatch::Handler handler( State& state)
         {
            common::message::dispatch::Handler handler;

            handler.add( handle::Begin{ state});
            handler.add( handle::Commit{ state});
            handler.add( handle::Rollback{ state});
            handler.add( handle::resource::reply::Connect{ state});
            handler.add( handle::resource::reply::Prepare{ state});
            handler.add( handle::resource::reply::Commit{ state});
            handler.add( handle::resource::reply::Rollback{ state});
            handler.add( handle::domain::Prepare{ state});
            handler.add( handle::domain::Commit{ state});
            handler.add( handle::domain::Rollback{ state});

            return handler;
         }


         void handleDispatch( State& state)
         {
            auto handler = local::handler( state);

            auto tmQueue = common::queue::non_blocking::reader( common::ipc::receive::queue());

            //
            // "make sure" mockup-queues has time to do their work...
            //
            common::process::sleep( std::chrono::milliseconds( 2));

            while( true)
            {
               auto message = tmQueue.next();

               if( message.empty())
                  return;

               handler.dispatch( message.front());
            }

         }
      } // local



      TEST( casual_transaction_manager, one_resource_connect__expect_no_broker_connect)
      {
         local::ipc::clear();

         common::mockup::ipc::Sender sender;

         State state = local::state( state::resource::Proxy::Instance::State::started);

         common::message::transaction::resource::connect::Reply reply;
         reply.id.queue_id = local::ipc::rm1_1.queue().id();
         reply.id.pid = local::ipc::rm1_1.pid();
         reply.resource = 1;
         reply.state = XA_OK;

         sender.add( common::ipc::receive::id(), reply);

         local::handleDispatch( state);

         auto brokerReader = common::queue::non_blocking::reader( common::mockup::ipc::broker::queue());
         common::message::transaction::Connected connected;
         EXPECT_FALSE( brokerReader( connected));
      }


      TEST( casual_transaction_manager, two_instance_of_one_resource_connect__expect_no_broker_connect)
      {
         local::ipc::clear();

         common::mockup::ipc::Sender sender;


         {
            common::message::transaction::resource::connect::Reply reply;
            reply.id.queue_id = local::ipc::rm2_1.queue().id();
            reply.id.pid = local::ipc::rm2_1.pid();
            reply.resource = 2;
            reply.state = XA_OK;
            sender.add( common::ipc::receive::id(), reply);
         }

         {
            common::message::transaction::resource::connect::Reply reply;
            reply.id.queue_id = local::ipc::rm2_2.queue().id();
            reply.id.pid = local::ipc::rm2_2.pid();
            reply.resource = 2;
            reply.state = XA_OK;
            sender.add( common::ipc::receive::id(), reply);
         }

         State state = local::state( state::resource::Proxy::Instance::State::started);
         local::handleDispatch( state);

         auto brokerReader = common::queue::non_blocking::reader( common::mockup::ipc::broker::queue());
         common::message::transaction::Connected connected;
         EXPECT_FALSE( brokerReader( connected));

      }


      TEST( casual_transaction_manager, one_instance_of_each_resource_connect__expect_broker_connect)
      {
         local::ipc::clear();

         common::mockup::ipc::Sender sender;


         {
            common::message::transaction::resource::connect::Reply reply;
            reply.id.queue_id = local::ipc::rm2_1.queue().id();
            reply.id.pid = local::ipc::rm2_1.pid();
            reply.resource = 2;
            reply.state = XA_OK;
            sender.add( common::ipc::receive::id(), reply);
         }

         {
            common::message::transaction::resource::connect::Reply reply;
            reply.id.queue_id = local::ipc::rm1_1.queue().id();
            reply.id.pid = local::ipc::rm1_1.pid();
            reply.resource = 1;
            reply.state = XA_OK;
            sender.add( common::ipc::receive::id(), reply);
         }


         State state = local::state( state::resource::Proxy::Instance::State::started);
         local::handleDispatch( state);

         // We expect broker reply
         auto brokerReader = common::queue::blocking::reader( common::mockup::ipc::broker::queue());
         common::message::transaction::Connected connected;
         brokerReader( connected);
         EXPECT_TRUE( connected.success);

      }

   } // transaction

} // casual
