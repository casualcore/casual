//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "service/forward/instance.h"
#include "service/unittest/utility.h"

#include "common/communication/instance.h"
#include "common/message/service.h"
#include "common/message/counter.h"

#include "domain/unittest/manager.h"

namespace casual
{

   namespace service
   {
      namespace local
      {
         namespace
         {
            namespace ipc
            {
               auto& inbound() { return common::communication::ipc::inbound::device();}
               auto& sm() { return common::communication::instance::outbound::service::manager::device();}
            } // ipc

            auto service_forward()
            {
               return common::communication::instance::fetch::handle( forward::instance::identity.id);
            }


            namespace configuration
            {
               constexpr auto base = R"(
domain:
   name: service-forward-domain

   servers:
      - path: ./bin/casual-service-manager    
)";
            }

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }

            auto message_count( const common::strong::ipc::id& ipc)
            {
               return common::communication::ipc::call( ipc, common::message::counter::Request{ common::process::handle()}).entries;
            }


         } // <unnamed>
      } // local

      TEST( service_forward, construction_destruction)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            auto domain = local::domain();
         });

      }


      TEST( service_forward, forward_call_TPNOTRAN)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         // advertise service2 
         casual::service::unittest::advertise( { "service2"});
         
         auto forward = local::service_forward();

         common::message::service::call::callee::Request request;
         {
            request.process = common::process::handle();
            request.service.name = "service2";
            request.trid = common::transaction::id::create( common::process::handle());
            request.flags = common::message::service::call::request::Flag::no_transaction;
         }

         // Send it to our forward, which will forward it to our self
         auto correlation = common::communication::device::blocking::send( forward.ipc, request);

         {
            common::message::service::call::callee::Request forwarded;
            common::communication::device::blocking::receive( local::ipc::inbound(), forwarded);

            EXPECT_TRUE( forwarded.correlation == correlation);
            EXPECT_TRUE( forwarded.trid == request.trid);
            EXPECT_TRUE( forwarded.flags == request.flags);
         }
      }


      TEST( service_forward, forward_call__missing_service__expect_error_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto forward = local::service_forward();

         auto request = []()
         {
            common::message::service::call::callee::Request request;
            request.process = common::process::handle();
            request.service.name = "non-existent-service";
            request.trid = common::transaction::id::create( common::process::handle());
            return request;
         }();


         // Send it to our forward, that will fail to lookup service and reply with error
         auto correlation = common::communication::device::blocking::send( forward.ipc, request);

         {
            // Expect error reply to caller
            common::message::service::call::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.correlation == correlation);
            EXPECT_TRUE( reply.transaction.trid == request.trid);
            EXPECT_TRUE( reply.code.result == common::code::xatmi::no_entry) << CASUAL_NAMED_VALUE( reply.code.result);
         }
      }

      TEST( service_forward, emulate_SF_lookup_request___timeout__expect_no_error_reply_from_SM)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      -  name: a
         execution:
            timeout:
               duration: 2ms
)";

         auto domain = local::domain( configuration);

         service::unittest::advertise( { "a"});

         // emulate that we're service-forward 
         {
            common::message::service::lookup::Request message{ common::process::handle()};
            message.requested = "a";
            message.context.semantic = decltype( message.context.semantic)::forward_request;

            auto reply = common::communication::ipc::call( local::ipc::sm(), message);
            EXPECT_TRUE( reply.process == common::process::handle());
         }

         // we sleep more than the timeout, and check that we didn't get a service-reply
         {
            common::process::sleep( std::chrono::milliseconds( 4));

            common::message::service::call::Reply reply;
            EXPECT_FALSE( common::communication::device::non::blocking::receive( local::ipc::inbound(), reply)) << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( service_forward, call_via_SF__timeout__expect_no_error_reply_to_SF_from_SM)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      -  name: a
         execution:
            timeout:
               duration: 2ms
)";

         auto domain = local::domain( configuration);

         service::unittest::advertise( { "a"});

         auto sf = local::service_forward();
         EXPECT_TRUE( sf);


         // call our self via service-forward.
         {
            common::message::service::call::callee::Request request;
            request.process = common::process::handle();
            using Flag = common::message::service::call::request::Flag;
            request.flags = common::flags::compose( Flag::no_reply, Flag::no_transaction);
            request.service.name = "a";

            EXPECT_TRUE( common::communication::device::blocking::send( sf.ipc, request));
         }

         // we sleep more than the timeout, and check that service-forward didn't get a service-reply
         {
            common::process::sleep( std::chrono::milliseconds( 4));

            auto counts = local::message_count( sf.ipc);

            EXPECT_FALSE( common::algorithm::find( counts, common::message::Type::service_reply)) << CASUAL_NAMED_VALUE( counts);
         }

      }

   } // service
} // casual
