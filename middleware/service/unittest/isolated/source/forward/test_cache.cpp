//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"


#include "service/forward/cache.h"

#include "common/mockup/domain.h"
#include "common/communication/ipc.h"


namespace casual
{
   using namespace common;
   namespace service
   {

      TEST( service_forward_cache, construction_destruction)
      {
         common::unittest::Trace trace;

         //
         // Take care of the connect
         //
         mockup::domain::minimal::Domain domain;

         EXPECT_NO_THROW({
            forward::Cache cache;
         });
      }


      TEST( service_forward_cache, forward_call_TPNOTRAN)
      {
         common::unittest::Trace trace;

         mockup::domain::minimal::Domain domain;

         mockup::ipc::Collector caller;

         message::service::call::callee::Request request;

         {
            request.process.pid = 1;
            request.process.queue = caller.id();
            request.service.name = "service2";
            request.trid = transaction::ID::create( process::handle());
            request.flags = message::service::call::request::Flag::no_transaction;
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
            EXPECT_TRUE( reply.status == common::code::xatmi::ok);
         }

         // make sure we quit
         communication::ipc::blocking::send( communication::ipc::inbound::id(), message::shutdown::Request{});

         cache_thread.join();

      }


      TEST( service_forward_cache, forward_call__missing_ipc_queue__expect_error_reply)
      {
         common::unittest::Trace trace;

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
            EXPECT_TRUE( reply.status == common::code::xatmi::service_error);

         }

         // make sure we quit
         communication::ipc::blocking::send( communication::ipc::inbound::id() , message::shutdown::Request{});

         cache_thread.join();

      }

   } // service
} // casual
