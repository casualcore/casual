//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "service/unittest/advertise.h"

#include "common/communication/instance.h"
#include "common/message/service.h"

#include "domain/manager/unittest/process.h"

namespace casual
{
   using namespace common;
   namespace service
   {
      namespace local
      {
         namespace
         {
            struct Domain 
            {
               casual::domain::manager::unittest::Process process{ { configuration}};

               auto forward() const
               {
                  return communication::instance::fetch::handle( communication::instance::identity::forward::cache);
               }

               static constexpr auto configuration = R"(
domain:
   name: service-forward-domain

   servers:
      - path: ./bin/casual-service-manager
        arguments [ --forward, ./bin/casual-service-forward]         
)";
            };
         } // <unnamed>
      } // local

      TEST( service_forward_cache, construction_destruction)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            local::Domain domain;
         });

         
      }


      TEST( service_forward_cache, forward_call_TPNOTRAN)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         // advertise service2 
         casual::service::unittest::advertise( { "service2"});
         
         auto forward = domain.forward();

         message::service::call::callee::Request request;
         {
            request.process = common::process::handle();
            request.service.name = "service2";
            request.trid = transaction::id::create( process::handle());
            request.flags = message::service::call::request::Flag::no_transaction;
         }

         // Send it to our forward, which will forward it to our self
         auto correlation = communication::ipc::blocking::send( forward.ipc, request);

         {
            message::service::call::callee::Request forwarded;
            communication::ipc::blocking::receive( communication::ipc::inbound::device(), forwarded);

            EXPECT_TRUE( forwarded.correlation == correlation);
            EXPECT_TRUE( forwarded.trid == request.trid);
            EXPECT_TRUE( forwarded.flags == request.flags);
         }
      }


      TEST( service_forward_cache, forward_call__missing_service__expect_error_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto forward = domain.forward();

         auto request = []()
         {
            message::service::call::callee::Request request;
            request.process = common::process::handle();
            request.service.name = "non-existent-service";
            request.trid = transaction::id::create( process::handle());
            return request;
         }();


         // Send it to our forward, that will fail to lookup service and reply with error
         auto correlation = communication::ipc::blocking::send( forward.ipc, request);

         {
            // Expect error reply to caller
            message::service::call::Reply reply;
            communication::ipc::blocking::receive( communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.correlation == correlation);
            EXPECT_TRUE( reply.transaction.trid == request.trid);
            EXPECT_TRUE( reply.code.result == common::code::xatmi::service_error);
         }
      }
   } // service
} // casual
