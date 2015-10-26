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

      TEST( casual_broker_forward_cache, construction_destruction)
      {
         //
         // Take care of the connect
         //
         mockup::domain::Domain domain;

         EXPECT_NO_THROW({
            forward::Cache cache;
         });
      }


      TEST( casual_broker_forward_cache, forward_call_TPNOREPLY_TPNOTRAN)
      {
         mockup::domain::Domain domain;

         mockup::ipc::Instance caller;

         message::service::call::callee::Request request;

         {
            request.process = caller.process();
            request.service.name = "service3_2ms_timout";
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
         // Send it to our forward
         //
         auto correlation = send( common::ipc::receive::id(), request);

         {
            common::queue::blocking::Reader receive{ caller.output()};

            message::service::call::Reply reply;
            receive( reply);

            EXPECT_TRUE( reply.correlation == correlation);
            EXPECT_TRUE( reply.transaction.trid == request.trid);
            EXPECT_TRUE( reply.error == 0);
         }

         // make sure we quit
         send( common::ipc::receive::id(), message::shutdown::Request{});

         cache_thread.join();

      }


      TEST( casual_broker_forward_cache, forward_call__missing_ipc_queue__expect_error_reply)
      {
         mockup::domain::Domain domain;

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
         auto correlation = send( common::ipc::receive::id(), request);

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
         send( common::ipc::receive::id(), message::shutdown::Request{});

         cache_thread.join();

      }

   } // broker
} // casual
