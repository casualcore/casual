//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "gateway/message.h"
#include "gateway/message/protocol.h"
#include "gateway/message/protocol/transform.h"
#include "gateway/documentation/protocol/example.h"

#include "common/serialize/native/network.h"
#include "common/algorithm/container.h"

namespace casual
{
   using namespace common;
   namespace gateway
   {
     
      TEST(gateway_protocol_transform, conversation_roundtrip_v1_2)
      {
         common::unittest::Trace trace;

         using Request = common::message::conversation::connect::callee::Request;
         using v1_2_Request = common::message::conversation::connect::v1_2::callee::Request;

         Request request;
         documentation::protocol::example::detail::fill(request);
         // Change value of duplex. The value "send" set by fill
         // corresponds to the default initialization value of duplex.
         // To be able to detect a missing "copy" of duplex we need a
         // "non-default" value.
         request.duplex = decltype(request.duplex)::receive;
         // Transform to V1_2. The transform::to requires a rvalue reference,
         // so the "transform source" may be "destroyed". Pass a copy so that
         // "request" can be used later.  
         auto v1_2_request =
            gateway::message::protocol::transform::to< v1_2_Request>( Request( request));
         // check
         // deadline.remaining should be copied to service.timeot.duration
         EXPECT_EQ(v1_2_request.service.timeout.duration, request.deadline.remaining);
         EXPECT_EQ(v1_2_request.parent, request.parent.service);
         // request.parent.span is lost
         // remaining fields verified after roundtrip
         
         // transform back to current version
         // transform::from also requires a rvalue reference argument, so its
         // argument may be destroyed.
         Request roundtrip_request =
            gateway::message::protocol::transform::from( std::move( v1_2_request)); 
         // Check if equal to/equivalent with original. We only check "non generic"
         // fields. There are expected differences after a roundtrip!
         // 
         // deadline.remaining:
         // This is service.timeout.duration in the V1_2 format of the message. A "roundtrip"
         // transformation should preserve the value.
         //
         // parent:
         // In V1_2 this is a member "parent" with a string with the parent service name.
         // in the current version it is a structure with members parent.service
         // and parent.span. A roundtrip transformation is expected to loose the span.
         EXPECT_EQ( roundtrip_request.service.name, request.service.name);
         EXPECT_EQ( roundtrip_request.deadline.remaining, request.deadline.remaining);
         EXPECT_EQ( roundtrip_request.parent.service, request.parent.service);
         decltype( request.parent.span) empty_span;
         EXPECT_EQ( roundtrip_request.parent.span, empty_span);
         EXPECT_EQ( roundtrip_request.trid, request.trid);
         EXPECT_EQ( roundtrip_request.duplex, request.duplex);
         EXPECT_EQ( roundtrip_request.buffer.type, request.buffer.type);
         EXPECT_EQ( roundtrip_request.buffer.data, request.buffer.data);
      }

      namespace local
      {
         namespace
         {
            template< typename T, typename M>
            auto roundtrip( M message)
            {
               return gateway::message::protocol::transform::from(
                  gateway::message::protocol::transform::to< T>( std::move( message)));
            }
         } // <unnamed>
      } // local

      TEST(gateway_protocol_transform, conversation_roundtrip_v1_5)
      {
         common::unittest::Trace trace;

         common::message::conversation::connect::callee::Request origin;
         origin.correlation = common::strong::correlation::id::generate();
         origin.execution = common::strong::execution::id::generate();
         origin.service.name = "service";
         origin.service.requested = "requested";
         origin.parent.service = "parent";
         origin.parent.span = strong::execution::span::id::generate();
         origin.deadline.remaining = std::chrono::seconds( 42);
         origin.trid = common::transaction::id::create();
         origin.pending = std::chrono::milliseconds( 37);
         origin.duplex = common::message::conversation::duplex::Type::receive; // ! 0
         origin.buffer.type = common::buffer::type::x_octet;
         origin.buffer.data = common::unittest::random::binary( 42);

         auto roundtrip = local::roundtrip< common::message::conversation::connect::v1_5::callee::Request>( origin);

         EXPECT_EQ( roundtrip.correlation, origin.correlation);
         EXPECT_EQ( roundtrip.execution, origin.execution);
         EXPECT_EQ( roundtrip.service, origin.service);
         EXPECT_EQ( roundtrip.parent, origin.parent);
         EXPECT_EQ( roundtrip.deadline.remaining, origin.deadline.remaining);
         EXPECT_EQ( roundtrip.trid, origin.trid);
         EXPECT_EQ( roundtrip.pending, origin.pending);
         EXPECT_EQ( roundtrip.duplex, origin.duplex);
         EXPECT_EQ( roundtrip.buffer, origin.buffer);
      }

      TEST(gateway_protocol_transform, conversation_send_roundtrip_v1_5)
      {
         common::unittest::Trace trace;

         common::message::conversation::callee::Send origin;
         origin.correlation = common::strong::correlation::id::generate();
         origin.execution = common::strong::execution::id::generate();
         origin.duplex = decltype( origin.duplex)::receive;
         origin.transaction_state = decltype( origin.transaction_state)::rollback;
         origin.code = common::service::Code{ .result = common::code::xatmi::ok, .user = 42};
         origin.buffer.type = common::buffer::type::x_octet;
         origin.buffer.data = common::unittest::random::binary( 64);

         auto roundtrip = local::roundtrip< common::message::conversation::v1_5::callee::Send>( origin);

         EXPECT_EQ( roundtrip.correlation, origin.correlation);
         EXPECT_EQ( roundtrip.execution, origin.execution);
         EXPECT_EQ( roundtrip.duplex, origin.duplex) << CASUAL_NAMED_VALUE( roundtrip.duplex);
         EXPECT_EQ( roundtrip.transaction_state, origin.transaction_state);
         // code.result will not be preserved in a roundtrip transformation of a non-terminated send message, 
         // it should be set to "absent" in this case.
         EXPECT_EQ( roundtrip.code.result, decltype( roundtrip.code.result)::absent) << CASUAL_NAMED_VALUE( roundtrip.code.result);
         EXPECT_EQ( roundtrip.code.user, origin.code.user) << CASUAL_NAMED_VALUE( roundtrip.code.user);
         EXPECT_EQ( roundtrip.buffer, origin.buffer);
      }
   }
}
