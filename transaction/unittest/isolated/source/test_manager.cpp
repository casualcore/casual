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
#include "common/message/dispatch.h"
#include "common/environment.h"




namespace casual
{

   namespace transaction
   {
      namespace local
      {

         namespace mockup
         {
            struct State : ::casual::transaction::State
            {
               using ::casual::transaction::State::State;

               using instance_type = common::mockup::ipc::Instance;


               void add( state::resource::Proxy&& proxy)
               {
                  for( std::size_t index = 0; index < proxy.concurency; ++index)
                  {
                     instance_type mockup( proxy.id * 100 + index);

                     state::resource::Proxy::Instance instance;
                     instance.id = proxy.id;
                     instance.process = mockup.process();
                     instance.state = state::resource::Proxy::Instance::State::started;

                     proxy.instances.push_back( std::move( instance));

                     mockups.emplace( mockup.process().pid, std::move( mockup));
                  }
                  resources.push_back( std::move( proxy));
               }

               instance_type& get( common::platform::pid_type pid)
               {
                  return mockups.at( pid);
               }

               std::map< common::platform::pid_type, instance_type> mockups;
            };

         } // mockup


         mockup::State state()
         {

            mockup::State state{ common::environment::directory::temporary() + "/test_transaction_log.db"};

            {
               state::resource::Proxy proxy;

               proxy.id = 1;
               proxy.key = "rm-mockup";
               proxy.concurency = 3;
               proxy.openinfo = "some open info 1";
               proxy.closeinfo = "some close info 1";

               state.add( std::move( proxy));
            }

            {
               state::resource::Proxy proxy;

               proxy.id = 2;
               proxy.key = "rm-mockup";
               proxy.concurency = 3;
               proxy.openinfo = "some open info 2";
               proxy.closeinfo = "some close info 2";

               state.add( std::move( proxy));
            }

            common::range::sort( common::range::make( state.resources));

            return state;
         }



         common::message::dispatch::Handler handler( State& state)
         {
            common::message::dispatch::Handler handler{
               handle::Begin{ state},
               handle::Commit{ state},
               handle::Rollback{ state},
               handle::resource::reply::Connect{ state},
               handle::resource::reply::Prepare{ state},
               handle::resource::reply::Commit{ state},
               handle::resource::reply::Rollback{ state},
               handle::domain::Prepare{ state},
               handle::domain::Commit{ state},
               handle::domain::Rollback{ state},
            };

            return handler;
         }


         void handleDispatch( State& state)
         {
            auto handler = local::handler( state);

            common::queue::non_blocking::Reader tmQueue( common::ipc::receive::queue());

            while( true)
            {
               common::process::sleep( std::chrono::milliseconds( 2));

               auto message = tmQueue.next();

               if( message.empty())
                  return;

               handler( message.front());
            }

         }
      } // local



      TEST( casual_transaction_manager, one_resource_connect__expect_no_broker_connect)
      {
         common::mockup::ipc::clear();

         // just a cache to keep queue writable
         common::mockup::ipc::Router router{ common::ipc::receive::id()};


         auto state = local::state();

         auto& rm = state.get( 100);

         common::message::transaction::resource::connect::Reply reply;
         reply.process = rm.process();
         reply.resource = 1;
         reply.state = XA_OK;

         common::queue::blocking::Writer( router.input())( reply);

         local::handleDispatch( state);

         common::queue::non_blocking::Reader broker( common::mockup::ipc::broker::queue().output());
         common::message::transaction::manager::Ready ready;
         EXPECT_FALSE( broker( ready));
      }


      TEST( casual_transaction_manager, two_instance_of_one_resource_connect__expect_no_broker_connect)
      {

         common::mockup::ipc::clear();

         // just a cache to keep queue writable
         common::mockup::ipc::Router router{ common::ipc::receive::id()};

         common::queue::blocking::Writer send( router.input());

         auto state = local::state();

         {
            auto& rm = state.get( 200);

            common::message::transaction::resource::connect::Reply reply;
            reply.process = rm.process();
            reply.resource = 2;
            reply.state = XA_OK;
            send( reply);
         }

         {
            auto& rm = state.get( 201);

            common::message::transaction::resource::connect::Reply reply;
            reply.process = rm.process();
            reply.resource = 2;
            reply.state = XA_OK;
            send( reply);
         }


         local::handleDispatch( state);

         common::queue::non_blocking::Reader broker( common::mockup::ipc::broker::queue().output());
         common::message::transaction::manager::Ready ready;
         EXPECT_FALSE( broker( ready));

      }



      TEST( casual_transaction_manager, one_instance_of_each_resource_connect__expect_broker_connect)
      {
         common::mockup::ipc::clear();

         // just a cache to keep queue writable
         common::mockup::ipc::Router router{ common::ipc::receive::id()};

         common::queue::blocking::Writer send( router.input());


         auto state = local::state();

         {
            auto& rm = state.get( 100);

            common::message::transaction::resource::connect::Reply reply;
            reply.process = rm.process();
            reply.resource = 1;
            reply.state = XA_OK;
            send( reply);
         }

         {
            auto& rm = state.get( 200);

            common::message::transaction::resource::connect::Reply reply;
            reply.process = rm.process();
            reply.resource = 2;
            reply.state = XA_OK;
            send( reply);
         }



         local::handleDispatch( state);

         // We expect broker reply
         common::queue::blocking::Reader broker( common::mockup::ipc::broker::queue().output());
         common::message::transaction::manager::Ready ready;
         broker( ready);
         EXPECT_TRUE( ready.success);

      }


   } // transaction

} // casual
