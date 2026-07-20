//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "service/forward/instance.h"
#include "service/unittest/utility.h"
#include "service/lookup.h"

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
            request.trid = common::transaction::id::create( common::process::id());
            using Flag = common::message::service::call::request::Flag;
            request.flags = Flag::no_reply | Flag::no_transaction;
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
            request.trid = common::transaction::id::create( common::process::id());
            return request;
         }();


         // Send it to our forward, that will fail to lookup service and reply with error
         auto correlation = common::communication::device::blocking::send( forward.ipc, request);

         {
            // Expect error reply to caller
            common::message::service::call::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.correlation == correlation);
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
            request.flags = Flag::no_reply | Flag::no_transaction;
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

      TEST( service_forward, advertise_a_b__reserve_a__no_reply_lookup_b___expect_pending_deadline_based_on_service_timeout)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   services:
      -  name: a
      -  name: b
         execution:
            timeout:
               duration: 10s
)");

         service::unittest::advertise( { "a", "b"});

         auto service_forward = local::service_forward();
         EXPECT_TRUE( service_forward);


         auto const deadline = common::chronology::time_point::clock::now() + std::chrono::seconds{ 2};
         
         auto reply_a = service::lookup::reply( service::Lookup{ "a", {}, {}, deadline});

         {
            EXPECT_TRUE( reply_a.state == decltype( reply_a.state)::idle);
            // deadline is based on the deadline we sent in lookup request, so it should be around 2 seconds.
            EXPECT_TRUE( reply_a.deadline.remaining <= std::chrono::seconds{ 2}) << CASUAL_NAMED_VALUE( reply_a);
            EXPECT_TRUE( reply_a.deadline.remaining >= std::chrono::seconds{ 1}) << CASUAL_NAMED_VALUE( reply_a);
            EXPECT_TRUE( reply_a.process == common::process::handle());
         }

         {
            using Semantic = decltype( service::lookup::Context::semantic);
            // send-and-forget lookup for b via service-forward, with the same deadline as a. deadline should be ignored for 
            // this lookup, since it's send-and-forget.
            auto reply_b = service::lookup::reply( service::Lookup{ "b", {}, { Semantic::no_reply}, deadline});
            EXPECT_TRUE( reply_b.state == decltype( reply_b.state)::idle);
            EXPECT_TRUE( ! reply_b.deadline.remaining ) << CASUAL_NAMED_VALUE( reply_b);
            EXPECT_TRUE( reply_b.process == service_forward);

            // Send call request to service-forward.
            common::message::service::call::callee::Request request{ common::process::handle()};
            request.service = reply_b.service;
            request.deadline = reply_b.deadline;
            request.flags = common::message::service::call::request::Flag::no_reply | common::message::service::call::request::Flag::no_transaction;
            request.correlation = reply_b.correlation;
            EXPECT_TRUE( common::communication::device::blocking::send( reply_b.process.ipc, request));

            // sm should only have pending deadline for the first lookup.
            auto state = casual::service::unittest::state();
            EXPECT_TRUE( state.pending.deadlines.size() == 1) << CASUAL_NAMED_VALUE( state.pending.deadlines);
            EXPECT_TRUE( state.pending.deadlines.at( 0).service == "a") << CASUAL_NAMED_VALUE( state.pending.deadlines);
         }

         // send ack for lookup_a
         service::unittest::send::ack( reply_a);

         // expect to get the call to b from service-forward
         {
            auto request = common::communication::ipc::receive< common::message::service::call::callee::Request>();
            EXPECT_TRUE( request.flags == (common::message::service::call::request::Flag::no_reply | common::message::service::call::request::Flag::no_transaction));
            EXPECT_TRUE( request.service.name == "b");
            // caller should be our self.
            EXPECT_TRUE( request.process == common::process::handle());

            // we should get a deadline based on the execution timeout of service b.
            EXPECT_TRUE( request.deadline.remaining <= std::chrono::seconds{ 10}) << CASUAL_NAMED_VALUE( request.deadline);
            EXPECT_TRUE( request.deadline.remaining >= std::chrono::seconds{ 9}) << CASUAL_NAMED_VALUE( request.deadline);

            // sm should have one pending deadline for the lookup to b, and it should be based on the execution timeout of b.
            auto state = casual::service::unittest::state();

            const auto now = common::chronology::time_point::clock::now();

            EXPECT_TRUE( state.pending.deadlines.size() == 1) << CASUAL_NAMED_VALUE( state.pending.deadlines);
            EXPECT_TRUE( state.pending.deadlines.at( 0).service == "b") << CASUAL_NAMED_VALUE( state.pending.deadlines);
            EXPECT_TRUE( state.pending.deadlines.at( 0).when <= now + std::chrono::seconds{ 10}) << CASUAL_NAMED_VALUE( state.pending.deadlines);
            EXPECT_TRUE( state.pending.deadlines.at( 0).when >= now + std::chrono::seconds{ 9}) << CASUAL_NAMED_VALUE( state.pending.deadlines);

            // send ack for b call for good measure.
            service::unittest::send::ack( request);
         }

         // some sanity check to make sure we don't have any pending deadlines in SM after the call is acked.
         {
            auto state = casual::service::unittest::state();
            EXPECT_TRUE( state.pending.deadlines.empty()) << CASUAL_NAMED_VALUE( state.pending.deadlines);
            EXPECT_TRUE( state.pending.requests.empty()) << CASUAL_NAMED_VALUE( state.pending.requests);
         }

      }

   } // service
} // casual
