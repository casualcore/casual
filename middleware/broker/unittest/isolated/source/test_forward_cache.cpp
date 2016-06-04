//!
//! test_forward_cache.cpp
//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"


#include "broker/forward/cache.h"

#include "common/mockup/domain.h"
#include "common/communication/ipc.h"
#include "common/trace.h"


namespace casual
{
   using namespace common;
   namespace broker
   {

      TEST( casual_broker_forward_cache, construction_destruction)
      {
         CASUAL_UNITTEST_TRACE();

         //
         // Take care of the connect
         //
         mockup::domain::minimal::Domain domain;

         EXPECT_NO_THROW({
            forward::Cache cache;
         });
      }


      TEST( casual_broker_forward_cache, forward_call_TPNOTRAN)
      {
         CASUAL_UNITTEST_TRACE();

         mockup::domain::minimal::Domain domain;

         mockup::ipc::Collector caller;

         message::service::call::callee::Request request;

         {
            request.process.pid = 1;
            request.process.queue = caller.id();
            request.service.name = "service3_2ms_timout";
            request.trid = transaction::ID::create( process::handle());
            request.flags = TPNOTRAN;
         }

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
         auto correlation = communication::ipc::blocking::send( communication::ipc::inbound::id(), request);

         {
            message::service::call::Reply reply;
            communication::ipc::blocking::receive( caller.output(), reply);


            EXPECT_TRUE( reply.correlation == correlation);
            EXPECT_TRUE( reply.transaction.trid == request.trid);
            EXPECT_TRUE( reply.error == 0);
         }

         // make sure we quit
         communication::ipc::blocking::send( communication::ipc::inbound::id(), message::shutdown::Request{});

         cache_thread.join();

      }


      TEST( casual_broker_forward_cache, forward_call__missing_ipc_queue__expect_error_reply)
      {
         CASUAL_UNITTEST_TRACE();

         mockup::domain::minimal::Domain domain;

         mockup::ipc::Collector caller;

         message::service::call::callee::Request request;

         {
            request.process.pid = 1;
            request.process.queue = caller.id();;
            request.service.name = "removed_ipc_queue";
            request.trid = transaction::ID::create( process::handle());
         }


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
         auto correlation = communication::ipc::blocking::send( communication::ipc::inbound::id(), request);

         {
            //
            // Expect error reply to caller
            //

            message::service::call::Reply reply;
            communication::ipc::blocking::receive( caller.output(), reply);

            EXPECT_TRUE( reply.correlation == correlation);
            EXPECT_TRUE( reply.transaction.trid == request.trid);
            EXPECT_TRUE( reply.error == TPESVCERR);

         }

         // make sure we quit
         communication::ipc::blocking::send( communication::ipc::inbound::id() , message::shutdown::Request{});

         cache_thread.join();

      }

   } // broker
} // casual
