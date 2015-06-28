//!
//! test_forward_cache.cpp
//!
//! Created on: Jun 28, 2015
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "broker/forward/cache.h"

#include "common/mockup/domain.h"
#include "common/queue.h"


namespace casual
{
   using namespace common;
   namespace broker
   {
      namespace local
      {
         namespace
         {

            struct Domain
            {
               //
               // Set up a "broker" that transforms request to replies, set destination to our receive queue
               //
               // Link 'output' from mockup-broker-queue to our "broker"
               //
               Domain()
                  : server{ 666},
                  broker{ ipc::receive::id(), mockup::create::broker({
                     mockup::create::lookup::reply( "service_1", server.input()),
                     mockup::create::lookup::reply( "timeout_2", server.input(), std::chrono::milliseconds{ 2}),
                     mockup::create::lookup::reply( "removed_ipc_queue", 0)
                  })},
                  link_broker_reply{ mockup::ipc::broker::queue().output().id(), broker.input()},
                  forward{ ipc::receive::id()}
               {

               }

               mockup::ipc::Instance server;
               mockup::ipc::Router broker;
               mockup::ipc::Link link_broker_reply;

               mockup::ipc::Router forward;
            };


         } // <unnamed>
      } // local

      TEST( casual_broker_forward_cache, construction_destruction)
      {
         //
         // Take care of the connect
         //
         local::Domain domain;

         EXPECT_NO_THROW({
            forward::Cache cache;
         });
      }


      TEST( casual_broker_forward_cache, forward_call_TPNOREPLY_TPNOTRAN)
      {
         local::Domain domain;

         mockup::ipc::Instance caller;

         message::service::call::callee::Request request;

         {
            request.process = caller.process();
            request.service.name = "timeout_2";
            request.trid = transaction::ID::create( caller.process());
            request.flags = TPNOREPLY | TPNOTRAN;
         }


         common::queue::blocking::Send send;


         //
         // Start the cache, witch will receive the request, and forward it to server
         //
         std::thread cache_thread{[](){
            forward::Cache cache;
            cache.start();
         }};


         //
         // Send it to our forward (that will rout it to the ipc-queue that the forward is listening to)
         //
         auto correlation = send( domain.forward.input(), request);

         {
            common::queue::blocking::Reader receive{ domain.server.output()};

            message::service::call::callee::Request forward;
            receive( forward);

            EXPECT_TRUE( forward.correlation == correlation);
            EXPECT_TRUE( forward.trid == request.trid);
            EXPECT_TRUE( forward.service.name == request.service.name);
            EXPECT_TRUE( forward.service.timeout == std::chrono::milliseconds{ 2}) << "timeout: " << forward.service.timeout.count();

         }

         // make sure we quit
         send( domain.forward.input(), message::shutdown::Request{});

         cache_thread.join();

      }


      TEST( casual_broker_forward_cache, forward_call__missing_ipc_queue__expect_error_reply)
      {
         local::Domain domain;

         mockup::ipc::Instance caller;

         message::service::call::callee::Request request;

         {
            request.process = caller.process();
            request.service.name = "removed_ipc_queue";
            request.trid = transaction::ID::create( caller.process());
         }


         //
         // Start the cache, witch will receive the request, and forward it to server
         //
         std::thread cache_thread{[](){
            forward::Cache cache;
            cache.start();
         }};

         common::queue::blocking::Send send;

         //
         // Send it to our forward (that will rout it to the ipc-queue that the forward is listening to)
         //
         auto correlation = send( domain.forward.input(), request);

         {
            //
            // Expect error reply to caller
            //
            common::queue::blocking::Reader receive{ caller.output()};

            message::service::call::Reply reply;
            receive( reply);

            EXPECT_TRUE( reply.correlation == correlation);
            EXPECT_TRUE( reply.transaction.trid == request.trid);
            EXPECT_TRUE( reply.error == TPESVCERR);

         }

         // make sure we quit
         send( domain.forward.input(), message::shutdown::Request{});

         cache_thread.join();

      }

   } // broker
} // casual
