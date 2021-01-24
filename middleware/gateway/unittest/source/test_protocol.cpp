//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "gateway/message.h"
#include "gateway/documentation/protocol/example.h"

#include "common/serialize/native/network.h"
#include "common/transcode.h"

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
      // in a comprehensable way anyway. Hence, we have to ensure the correctness by 
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
            template< typename M, typename B>
            void compare( M&& message, B&& base64)
            {
               serialize::native::binary::network::Writer writer;
               writer << message;
               
               auto result = transcode::base64::encode( writer.consume());

               EXPECT_TRUE( base64 == result) << result;
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
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAAAEAAAAAAAAD6A==)";
         local::compare( local::fill< gateway::message::domain::connect::Request>(), expected);
      }

      TEST( gateway_protocol_v1, connect_reply)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAA+g=)";
         local::compare( local::fill< gateway::message::domain::connect::Reply>(), expected);
      }

      TEST( gateway_protocol_v1_1, disconnect_request)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YA==)";
         local::compare( local::fill< gateway::message::domain::disconnect::Request>(), expected);
      }

      TEST( gateway_protocol_v1_1, disconnect_reply)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YA==)";
         local::compare( local::fill< gateway::message::domain::disconnect::Reply>(), expected);
      }

      TEST( gateway_protocol_v1, discover_request)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAAAMAAAAAAAAACHNlcnZpY2UxAAAAAAAAAAhzZXJ2aWNlMgAAAAAAAAAIc2VydmljZTMAAAAAAAAAAwAAAAAAAAAGcXVldWUxAAAAAAAAAAZxdWV1ZTIAAAAAAAAABnF1ZXVlMw==)";
         local::compare( local::fill< common::message::gateway::domain::discover::Request>(), expected);
      }

      TEST( gateway_protocol_v1, discover_reply)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YOL2t8N/c0oJgqCrFYGyH6UAAAAAAAAACGRvbWFpbiBCAAAAAAAAAAEAAAAAAAAACHNlcnZpY2UxAAAAAAAAAAdleGFtcGxlAAEAAAAU9GsEAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAABnF1ZXVlMQAAAAAAAAAK)";
         local::compare( local::fill< common::message::gateway::domain::discover::Reply>(), expected);
      }

      TEST( gateway_protocol_v1, call_request)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAIc2VydmljZTEAAAAJx2UkAAAAAAAAAAAOcGFyZW50LXNlcnZpY2UAAAAAAAAAKgAAAAAAAAAQAAAAAAAAABBbbBv28ktIDb283vVMOghRW2wb9vJLSA29vN71TDoIUgAAAAAAAAAEAAAAAAAAAAguYmluYXJ5LwAAAAAAAACAgIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6vsLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f4OHi4+Tl5ufo6err7O3u7/Dx8vP09fb3+Pn6+/z9/v8=)";
         local::compare( local::fill< common::message::service::call::callee::Request>(), expected);
      }

      TEST( gateway_protocol_v1, call_reply)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAsAAAAAAAAAKgAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAAAAAAAAILmJpbmFyeS8AAAAAAAAAgICBgoOEhYaHiImKi4yNjo+QkZKTlJWWl5iZmpucnZ6foKGio6SlpqeoqaqrrK2ur7CxsrO0tba3uLm6u7y9vr/AwcLDxMXGx8jJysvMzc7P0NHS09TV1tfY2drb3N3e3+Dh4uPk5ebn6Onq6+zt7u/w8fLz9PX29/j5+vv8/f7/)";
         local::compare( local::fill< common::message::service::call::Reply>(), expected);
      }


      TEST( gateway_protocol_v1, resource_prepare_request)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAAAAAAA)";
         local::compare( local::fill< common::message::transaction::resource::prepare::Request>(), expected);
      }

      TEST( gateway_protocol_v1, resource_prepare_reply)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAA=)";
         local::compare( local::fill< common::message::transaction::resource::prepare::Reply>(), expected);
      }

      TEST( gateway_protocol_v1, resource_commit_request)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAAAAAAA)";
         local::compare( local::fill< common::message::transaction::resource::commit::Request>(), expected);
      }

      TEST( gateway_protocol_v1, resource_commit_reply)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAA=)";
         local::compare( local::fill< common::message::transaction::resource::commit::Reply>(), expected);
      }

      TEST( gateway_protocol_v1, resource_rollback_request)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAAAAAAA)";
         local::compare( local::fill< common::message::transaction::resource::rollback::Request>(), expected);
      }

      TEST( gateway_protocol_v1, resource_rollback_reply)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAA=)";
         local::compare( local::fill< common::message::transaction::resource::rollback::Reply>(), expected);
      }

      TEST( gateway_protocol_v1, enqueue_request)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAGcXVldWVBAAAAAAAAACoAAAAAAAAAEAAAAAAAAAAQW2wb9vJLSA29vN71TDoIUVtsG/byS0gNvbze9Uw6CFLm/Z/PhqxH9KUlL1l+JfxqAAAAAAAAABVwcm9wZXJ0eSAxOnByb3BlcnR5IDIAAAAAAAAABnF1ZXVlQhWlY3jTqfCgAAAAAAAAAAguYmluYXJ5LwAAAAAAAACAgIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6vsLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f4OHi4+Tl5ufo6err7O3u7/Dx8vP09fb3+Pn6+/z9/v8=)";  
         local::compare( local::fill< queue::ipc::message::group::enqueue::Request>(), expected);
      }

      TEST( gateway_protocol_v1, enqueue_reply)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4c=)";
         local::compare( local::fill< queue::ipc::message::group::enqueue::Reply>(), expected);
      }


      TEST( gateway_protocol_v1, dequeue_request)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAGcXVldWVBAAAAAAAAACoAAAAAAAAAEAAAAAAAAAAQW2wb9vJLSA29vN71TDoIUVtsG/byS0gNvbze9Uw6CFIAAAAAAAAAFXByb3BlcnR5IDE6cHJvcGVydHkgMjFdrMYYLkwSv5h376kky4cA)";
         local::compare( local::fill< queue::ipc::message::group::dequeue::Request>(), expected);
      }

      TEST( gateway_protocol_v1, dequeue_reply)
      {
         constexpr auto expected = R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAABUy+LbBV2Tcqf6CowAt5XngAAAAAAAAAVcHJvcGVydHkgMTpwcm9wZXJ0eSAyAAAAAAAAAAZxdWV1ZUIVpWN406nwoAAAAAAAAAAGLmpzb24vAAAAAAAAAAJ7fQAAAAAAAAABFaVjeNOp8KA=)";
         local::compare( local::fill< queue::ipc::message::group::dequeue::Reply>(), expected);
      }

   }
}
