//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "gateway/message.h"
#include "gateway/message/protocol.h"
#include "gateway/documentation/protocol/example.h"

#include "common/serialize/native/network.h"
#include "common/transcode.h"
#include "common/algorithm/container.h"

namespace casual
{
   using namespace common;
   namespace gateway
   {
      // The tests down below ensures that any change to the protocol (unwanted) will
      // be detected.
      // Since we use a subset of attributes to propagate over the wire, it's rather
      // easy to miss adding a attribute for internal use, that leaks out to the
      // interdomain protocol (evidently).
      // The tests of their own does not guarantee that the protocol is correct, not 
      // in a comprehensible way anyway. Hence, we have to ensure the correctness by 
      // other means.
      // 
      // semantics for the tests: 
      //   * fill the wanted message with _example information_
      //   * serialize to network binary
      //   * base64 encode the binary representation
      //   * compare with expected base64 representation


      template <typename M>
      struct gateway_protocol : public ::testing::Test
      {
         using message_type = M;
      };

      using message_types = ::testing::Types<
         gateway::message::domain::connect::Request,
         gateway::message::domain::connect::Reply,
         casual::domain::message::discovery::Request,
         casual::domain::message::discovery::v1_3::Reply,
         casual::domain::message::discovery::Reply,
         casual::domain::message::discovery::topology::implicit::Update,
         common::message::service::call::v1_2::callee::Request,
         common::message::service::call::callee::Request,
         common::message::service::call::v1_2::Reply,
         common::message::service::call::Reply,
         common::message::conversation::connect::callee::Request,
         common::message::conversation::connect::v1_2::callee::Request,
         common::message::conversation::connect::Reply,
         common::message::conversation::callee::Send,
         common::message::conversation::Disconnect,
         common::message::transaction::resource::prepare::Request,
         common::message::transaction::resource::prepare::Reply,
         common::message::transaction::resource::commit::Request,
         common::message::transaction::resource::commit::Reply,
         common::message::transaction::resource::rollback::Request,
         common::message::transaction::resource::rollback::Reply,
         queue::ipc::message::group::enqueue::Request,
         queue::ipc::message::group::enqueue::v1_2::Reply,
         queue::ipc::message::group::enqueue::Reply,
         queue::ipc::message::group::dequeue::Request,
         queue::ipc::message::group::dequeue::v1_2::Reply,
         queue::ipc::message::group::dequeue::Reply,
         gateway::message::domain::disconnect::Request,
         gateway::message::domain::disconnect::Reply
      >;

      TYPED_TEST_SUITE( gateway_protocol, message_types);

      TYPED_TEST( gateway_protocol, validate_binary_representation)
      {
         using message_type = TestFixture::message_type;
         auto message = documentation::protocol::example::message< message_type>();
         auto base64 = documentation::protocol::example::representation::base64< message_type>();

         serialize::native::binary::network::Writer writer;
         writer << message;
         
         auto result = transcode::base64::encode( writer.consume());

         EXPECT_TRUE( base64 == result) 
            << "result:   " << result << "\n" 
            << "expected: " << base64 << '\n' 
            << CASUAL_NAMED_VALUE( message);
      }

      static_assert( gateway::message::protocol::version< gateway::message::domain::disconnect::Reply>().min == gateway::message::protocol::Version::v1_1);
      static_assert( gateway::message::protocol::version< queue::ipc::message::group::enqueue::v1_2::Reply>().min == gateway::message::protocol::Version::v1_0);
      static_assert( gateway::message::protocol::version< queue::ipc::message::group::enqueue::v1_2::Reply>().max == gateway::message::protocol::Version::v1_2);
      static_assert( gateway::message::protocol::version< queue::ipc::message::group::enqueue::Reply>().min == gateway::message::protocol::Version::v1_3);
      static_assert( gateway::message::protocol::version< queue::ipc::message::group::dequeue::v1_2::Reply>().min == gateway::message::protocol::Version::v1_0);
      static_assert( gateway::message::protocol::version< queue::ipc::message::group::dequeue::v1_2::Reply>().max == gateway::message::protocol::Version::v1_2);
      static_assert( gateway::message::protocol::version< queue::ipc::message::group::dequeue::Reply>().min == gateway::message::protocol::Version::v1_3);
      static_assert( gateway::message::protocol::version< gateway::message::domain::disconnect::Request>().min == gateway::message::protocol::Version::v1_1);
      static_assert( gateway::message::protocol::version< gateway::message::domain::disconnect::Reply>().min == gateway::message::protocol::Version::v1_1);

        /* 
         // connect request
         // current 1.4
         {
            constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAAAUAAAAAAAAD7AAAAAAAAAPrAAAAAAAAA+oAAAAAAAAD6QAAAAAAAAPo)";
            local::compare( message, expected);
         }

         // 1.3
         {
            algorithm::container::erase( message.versions, message::protocol::Version::v1_4);
            constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAAAQAAAAAAAAD6wAAAAAAAAPqAAAAAAAAA+kAAAAAAAAD6A==)";
            local::compare( message, expected);
         }

         // 1.2
         {
            algorithm::container::erase( message.versions, message::protocol::Version::v1_3);
            constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAAAMAAAAAAAAD6gAAAAAAAAPpAAAAAAAAA+g=)";
            local::compare( message, expected);
         }
         
      */
   


   }
}
