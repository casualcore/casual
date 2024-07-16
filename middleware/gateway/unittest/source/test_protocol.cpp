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

      namespace local
      {
         namespace
         {
            template< typename M>
            void compare( M&& message, std::string_view base64)
            {
               serialize::native::binary::network::Writer writer;
               writer << message;
               
               auto result = transcode::base64::encode( writer.consume());

               EXPECT_TRUE( base64 == result) 
                  << "result:   " << result << "\n" 
                  << "expected: " << base64 << '\n' 
                  << CASUAL_NAMED_VALUE( message);
            }

            template< typename M>
            auto fill()
            {
               return documentation::protocol::example::message< M>();
            }

         } // <unnamed>
      } // local



      TEST( gateway_protocol_v1, connect_request)
      {
         auto message = local::fill< gateway::message::domain::connect::Request>();
         
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
      }

      TEST( gateway_protocol_v1, connect_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAA+g=)";
         local::compare( local::fill< gateway::message::domain::connect::Reply>(), expected);
      }

      TEST( gateway_protocol_v1, discover_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAAAMAAAAAAAAACHNlcnZpY2UxAAAAAAAAAAhzZXJ2aWNlMgAAAAAAAAAIc2VydmljZTMAAAAAAAAAAwAAAAAAAAAGcXVldWUxAAAAAAAAAAZxdWV1ZTIAAAAAAAAABnF1ZXVlMw==)";
         local::compare( local::fill< casual::domain::message::discovery::Request>(), expected);
      }

      TEST( gateway_protocol_v1, discover_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YOL2t8N/c0oJgqCrFYGyH6UAAAAAAAAACGRvbWFpbiBCAAAAAAAAAAEAAAAAAAAACHNlcnZpY2UxAAAAAAAAAAdleGFtcGxlAAEAAAAU9GsEAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAABnF1ZXVlMQAAAAAAAAAK)";
         local::compare( local::fill< casual::domain::message::discovery::v1_3::Reply>(), expected);
      }

      TEST( gateway_protocol_v4, discover_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YOL2t8N/c0oJgqCrFYGyH6UAAAAAAAAACGRvbWFpbiBCAAAAAAAAAAEAAAAAAAAACHNlcnZpY2UxAAAAAAAAAAdleGFtcGxlAAEAAAAU9GsEAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAABnF1ZXVlMQAAAAAAAAAKAAAAAAA9CQABAA==)";
         local::compare( local::fill< casual::domain::message::discovery::Reply>(), expected);
      }

      TEST( gateway_protocol_v2, call_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAIc2VydmljZTEAAAAJx2UkAAAAAAAAAAAOcGFyZW50LXNlcnZpY2UAAAAAAAAAKgAAAAAAAAAQAAAAAAAAABBbbBv28ktIDb283vVMOghRW2wb9vJLSA29vN71TDoIUgAAAAAAAAAEAAAAAAAAAAguYmluYXJ5LwAAAAAAAACAgIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6vsLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f4OHi4+Tl5ufo6err7O3u7/Dx8vP09fb3+Pn6+/z9/v8=)";
         local::compare( local::fill< common::message::service::call::v1_2::callee::Request>(), expected);
      }

      TEST( gateway_protocol_v3, call_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAIc2VydmljZTEAAAAJx2UkAICBgoOEhYaHAAAAAAAAAA5wYXJlbnQtc2VydmljZQAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAAAAAAAQAAAAAAAAACC5iaW5hcnkvAAAAAAAAAICAgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==)";
         local::compare( local::fill< common::message::service::call::callee::Request>(), expected);
      }

      TEST( gateway_protocol_v2, call_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAsAAAAAAAAAKgAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAAAAAAAAILmJpbmFyeS8AAAAAAAAAgICBgoOEhYaHiImKi4yNjo+QkZKTlJWWl5iZmpucnZ6foKGio6SlpqeoqaqrrK2ur7CxsrO0tba3uLm6u7y9vr/AwcLDxMXGx8jJysvMzc7P0NHS09TV1tfY2drb3N3e3+Dh4uPk5ebn6Onq6+zt7u/w8fLz9PX29/j5+vv8/f7/)";
         local::compare( local::fill< common::message::service::call::v1_2::Reply>(), expected);
      }

      TEST( gateway_protocol_v3, call_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAsAAAAAAAAAKgAAAAAAAAAACC5iaW5hcnkvAAAAAAAAAICAgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==)";
         local::compare( local::fill< common::message::service::call::Reply>(), expected);
      }


      TEST( gateway_protocol_v3, conversation_connect_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAIc2VydmljZTEAAAAJx2UkAICBgoOEhYaHAAAAAAAAAA5wYXJlbnQtc2VydmljZQAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAAAAAAAAACC5iaW5hcnkvAAAAAAAAAICAgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==)";
         local::compare( local::fill< common::message::conversation::connect::callee::Request>(), expected);
      }

      TEST( gateway_protocol_v2, conversation_connect_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAIc2VydmljZTEAAAAJx2UkAAAAAAAAAAAOcGFyZW50LXNlcnZpY2UAAAAAAAAAKgAAAAAAAAAQAAAAAAAAABBbbBv28ktIDb283vVMOghRW2wb9vJLSA29vN71TDoIUgAAAAAAAAAAAAguYmluYXJ5LwAAAAAAAACAgIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6vsLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f4OHi4+Tl5ufo6err7O3u7/Dx8vP09fb3+Pn6+/z9/v8=)";
         local::compare( local::fill< common::message::conversation::connect::v1_2::callee::Request>(), expected);
      }



      TEST( gateway_protocol_v1, resource_prepare_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAAAAAAA)";
         local::compare( local::fill< common::message::transaction::resource::prepare::Request>(), expected);
      }

      TEST( gateway_protocol_v1, resource_prepare_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAA=)";
         local::compare( local::fill< common::message::transaction::resource::prepare::Reply>(), expected);
      }

      TEST( gateway_protocol_v1, resource_commit_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAAAAAAA)";
         local::compare( local::fill< common::message::transaction::resource::commit::Request>(), expected);
      }

      TEST( gateway_protocol_v1, resource_commit_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAA=)";
         local::compare( local::fill< common::message::transaction::resource::commit::Reply>(), expected);
      }

      TEST( gateway_protocol_v1, resource_rollback_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAAAAAAA)";
         local::compare( local::fill< common::message::transaction::resource::rollback::Request>(), expected);
      }

      TEST( gateway_protocol_v1, resource_rollback_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAA=)";
         local::compare( local::fill< common::message::transaction::resource::rollback::Reply>(), expected);
      }

      TEST( gateway_protocol_v1, enqueue_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAGcXVldWVBAAAAAAAAACoAAAAAAAAAEAAAAAAAAAAQW2wb9vJLSA29vN71TDoIUVtsG/byS0gNvbze9Uw6CFLm/Z/PhqxH9KUlL1l+JfxqAAAAAAAAABVwcm9wZXJ0eSAxOnByb3BlcnR5IDIAAAAAAAAABnF1ZXVlQhWlY3jTqfCgAAAAAAAAAAguYmluYXJ5LwAAAAAAAACAgIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6vsLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f4OHi4+Tl5ufo6err7O3u7/Dx8vP09fb3+Pn6+/z9/v8=)";  
         local::compare( local::fill< queue::ipc::message::group::enqueue::Request>(), expected);
      }

      TEST( gateway_protocol_v1_2, enqueue_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4c=)";
         using message_type = queue::ipc::message::group::enqueue::v1_2::Reply;
         local::compare( local::fill< message_type>(), expected);

         static_assert( gateway::message::protocol::version< message_type>().min == gateway::message::protocol::Version::v1_0);
         static_assert( gateway::message::protocol::version< message_type>().max == gateway::message::protocol::Version::v1_2);
      }

      TEST( gateway_protocol_v1_3, enqueue_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YByY2UvSmkBhnYpzABnciR8AAAAe)";
         using message_type = queue::ipc::message::group::enqueue::Reply;
         local::compare( local::fill< message_type>(), expected);

         static_assert( gateway::message::protocol::version< message_type>().min == gateway::message::protocol::Version::v1_3);
      }


      TEST( gateway_protocol_v1, dequeue_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAGcXVldWVBAAAAAAAAACoAAAAAAAAAEAAAAAAAAAAQW2wb9vJLSA29vN71TDoIUVtsG/byS0gNvbze9Uw6CFIAAAAAAAAAFXByb3BlcnR5IDE6cHJvcGVydHkgMjFdrMYYLkwSv5h376kky4cA)";
         local::compare( local::fill< queue::ipc::message::group::dequeue::Request>(), expected);
      }

      TEST( gateway_protocol_v1_2, dequeue_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAABUy+LbBV2Tcqf6CowAt5XngAAAAAAAAAVcHJvcGVydHkgMTpwcm9wZXJ0eSAyAAAAAAAAAAZxdWV1ZUIVpWN406nwoAAAAAAAAAAGLmpzb24vAAAAAAAAAAJ7fQAAAAAAAAABFaVjeNOp8KA=)";
         using message_type = queue::ipc::message::group::dequeue::v1_2::Reply;
         local::compare( local::fill< message_type>(), expected);

         static_assert( gateway::message::protocol::version< message_type>().min == gateway::message::protocol::Version::v1_0);
         static_assert( gateway::message::protocol::version< message_type>().max == gateway::message::protocol::Version::v1_2);

      }

      TEST( gateway_protocol_v1_3, dequeue_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YAFTL4tsFXZNyp/oKjAC3leeAAAAAAAAABVwcm9wZXJ0eSAxOnByb3BlcnR5IDIAAAAAAAAABnF1ZXVlQhWlY3jTqfCgAAAAAAAAAAYuanNvbi8AAAAAAAAAAnt9AAAAAAAAAAEVpWN406nwoAAAABQ=)";
         using message_type = queue::ipc::message::group::dequeue::Reply;
         local::compare( local::fill< message_type>(), expected);

         static_assert( gateway::message::protocol::version< message_type>().min == gateway::message::protocol::Version::v1_3);
      }

      TEST( gateway_protocol_v1_1, disconnect_request)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YA==)";
         local::compare( local::fill< gateway::message::domain::disconnect::Request>(), expected);

         static_assert( gateway::message::protocol::version< gateway::message::domain::disconnect::Request>().min == gateway::message::protocol::Version::v1_1);
         
      }

      TEST( gateway_protocol_v1_1, disconnect_reply)
      {
         constexpr std::string_view expected = R"(cHPL9BRESkGHswCG8UP8YA==)";
         local::compare( local::fill< gateway::message::domain::disconnect::Reply>(), expected);

         static_assert( gateway::message::protocol::version< gateway::message::domain::disconnect::Reply>().min == gateway::message::protocol::Version::v1_1);
      }


   }
}
