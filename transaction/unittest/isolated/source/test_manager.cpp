//!
//! test_manager.cpp
//!
//! Created on: Jan 6, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "transaction/manager/handle.h"


#include "common/mockup.h"
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
            const common::platform::queue_id_type broker = 1;
            const common::platform::queue_id_type tm = 10;

            const common::platform::queue_id_type rm1_1 = 101;
            const common::platform::queue_id_type rm1_2 = 102;
            const common::platform::queue_id_type rm1_3 = 103;

            const common::platform::queue_id_type rm2_1 = 201;
            const common::platform::queue_id_type rm2_2 = 202;
            const common::platform::queue_id_type rm2_3 = 203;


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
            instance.server.queue_id = ipc::rm1_1;
            instance.server.pid = ipc::rm1_1;
            instance.state = mode;

            proxy.instances.push_back( instance);

            instance.server.queue_id = ipc::rm1_2;
            instance.server.pid = ipc::rm1_2;
            proxy.instances.push_back( instance);

            instance.server.queue_id = ipc::rm1_3;
            instance.server.pid = ipc::rm1_3;
            proxy.instances.push_back( instance);

            state.resources.push_back( proxy);

            proxy.instances.clear();
            proxy.id = 2;
            proxy.openinfo = "some open info 2";
            proxy.closeinfo = "some close info 2";

            instance.id = 2;
            instance.server.queue_id = ipc::rm2_1;
            instance.server.pid = ipc::rm2_1;
            proxy.instances.push_back( instance);

            instance.server.queue_id = ipc::rm2_2;
            instance.server.pid = ipc::rm2_2;
            proxy.instances.push_back( instance);


            instance.server.queue_id = ipc::rm2_3;
            instance.server.pid = ipc::rm2_3;
            proxy.instances.push_back( instance);

            state.resources.push_back( proxy);

            common::range::sort( common::range::make( state.resources));

            return state;
         }

         namespace mockup
         {
            namespace queue
            {
               namespace blocking
               {

                  using Reader = common::mockup::queue::blocking::basic_reader< policy::Manager>;
                  using Writer = common::mockup::queue::blocking::basic_writer< policy::Manager>;

               } // blocking

               namespace non_blocking
               {
                  using Reader = common::mockup::queue::non_blocking::basic_reader< policy::Manager>;
                  using Writer = common::mockup::queue::non_blocking::basic_writer< policy::Manager>;

               } // non_blocking

               struct Policy
               {
                  using block_writer = blocking::Writer;
                  using non_block_writer = non_blocking::Writer;
               };

            } // queue

         } // mockup


         common::message::dispatch::Handler handler( State& state)
         {
            common::message::dispatch::Handler handler;

            handler.add( handle::Begin{ state});
            handler.add( handle::basic_commit< mockup::queue::Policy>{ state});
            handler.add( handle::Rollback{ state});
            handler.add( handle::resource::reply::basic_connect< mockup::queue::Policy>( state, ipc::broker));
            handler.add( handle::resource::reply::Prepare{ state});
            handler.add( handle::resource::reply::Commit{ state});
            handler.add( handle::resource::reply::Rollback{ state});
            handler.add( handle::domain::basic_prepare< mockup::queue::Policy>{ state});
            handler.add( handle::domain::basic_commit< mockup::queue::Policy>{ state});
            handler.add( handle::domain::basic_rollback< mockup::queue::Policy>{ state});

            return handler;
         }


         void readUntilEmpty( State& state)
         {
            auto handler = local::handler( state);

            local::mockup::queue::non_blocking::Reader tmQueue{ local::ipc::tm, state};

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
         common::mockup::queue::clearAllQueues();

         State state = local::state( state::resource::Proxy::Instance::State::started);
         local::mockup::queue::non_blocking::Writer writer{ local::ipc::tm, state};

         common::message::transaction::resource::connect::Reply reply;
         reply.id.queue_id = local::ipc::rm1_1;
         reply.id.pid = local::ipc::rm1_1;
         reply.resource = 1;
         reply.state = XA_OK;
         writer( reply);

         local::readUntilEmpty( state);

         common::mockup::queue::non_blocking::Reader brokerReader( local::ipc::broker);
         common::message::transaction::Connected connected;
         EXPECT_FALSE( brokerReader( connected));
      }


      TEST( casual_transaction_manager, two_instance_of_one_resource_connect__expect_no_broker_connect)
      {
         common::mockup::queue::clearAllQueues();

         common::mockup::queue::non_blocking::Writer writer{ local::ipc::tm};


         {
            common::message::transaction::resource::connect::Reply reply;
            reply.id.queue_id = local::ipc::rm2_1;
            reply.id.pid = local::ipc::rm2_1;
            reply.resource = 2;
            reply.state = XA_OK;
            writer( reply);
         }

         {
            common::message::transaction::resource::connect::Reply reply;
            reply.id.queue_id = local::ipc::rm2_2;
            reply.id.pid = local::ipc::rm2_2;
            reply.resource = 2;
            reply.state = XA_OK;
            writer( reply);
         }

         State state = local::state( state::resource::Proxy::Instance::State::started);
         local::readUntilEmpty( state);

         common::mockup::queue::non_blocking::Reader brokerReader( local::ipc::broker);
         common::message::transaction::Connected connected;
         EXPECT_FALSE( brokerReader( connected));

      }


      TEST( casual_transaction_manager, one_instance_of_each_resource_connect__expect_broker_connect)
      {
         common::mockup::queue::clearAllQueues();

         common::mockup::queue::non_blocking::Writer writer{ local::ipc::tm};


         {
            common::message::transaction::resource::connect::Reply reply;
            reply.id.queue_id = local::ipc::rm2_1;
            reply.id.pid = local::ipc::rm2_1;
            reply.resource = 2;
            reply.state = XA_OK;
            writer( reply);
         }

         {
            common::message::transaction::resource::connect::Reply reply;
            reply.id.queue_id = local::ipc::rm1_1;
            reply.id.pid = local::ipc::rm1_1;
            reply.resource = 1;
            reply.state = XA_OK;
            writer( reply);
         }

         State state = local::state( state::resource::Proxy::Instance::State::started);
         local::readUntilEmpty( state);

         // We expect broker reply
         common::mockup::queue::non_blocking::Reader brokerReader( local::ipc::broker);
         common::message::transaction::Connected connected;
         ASSERT_TRUE( brokerReader( connected));
         EXPECT_TRUE( connected.success);

      }

   } // transaction

} // casual
