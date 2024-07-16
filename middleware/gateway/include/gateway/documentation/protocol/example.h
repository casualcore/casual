//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/message.h"

namespace casual
{
   namespace gateway::documentation::protocol::example
   {
      namespace detail
      {
         void fill( gateway::message::domain::connect::Request& message);
         void fill( gateway::message::domain::connect::Reply& message);

         void fill( gateway::message::domain::disconnect::Request& message);
         void fill( gateway::message::domain::disconnect::Reply& message);

         void fill( casual::domain::message::discovery::Request& message);
         void fill( casual::domain::message::discovery::Reply& message);
         void fill( casual::domain::message::discovery::v1_3::Reply& message);
         void fill( casual::domain::message::discovery::topology::implicit::Update& message);

         void fill( common::message::service::call::callee::Request& message);
         void fill( common::message::service::call::v1_2::callee::Request& message);
         void fill( common::message::service::call::Reply& message);
         void fill( common::message::service::call::v1_2::Reply& message);

         void fill( common::message::conversation::connect::callee::Request& message);
         void fill( common::message::conversation::connect::v1_2::callee::Request& message);
         void fill( common::message::conversation::connect::Reply& message);

         void fill( common::message::conversation::callee::Send& message);
         void fill( common::message::conversation::Disconnect& message);

         void fill( casual::queue::ipc::message::group::enqueue::Request& message);
         void fill( casual::queue::ipc::message::group::enqueue::Reply& message);
         void fill( casual::queue::ipc::message::group::enqueue::v1_2::Reply& message);

         void fill( casual::queue::ipc::message::group::dequeue::Request& message);
         void fill( casual::queue::ipc::message::group::dequeue::Reply& message);
         void fill( casual::queue::ipc::message::group::dequeue::v1_2::Reply& message);

         void fill( common::message::transaction::resource::prepare::Request& message);
         void fill( common::message::transaction::resource::prepare::Reply& message);

         void fill( common::message::transaction::resource::commit::Request& message);
         void fill( common::message::transaction::resource::commit::Reply& message);

         void fill( common::message::transaction::resource::rollback::Request& message);
         void fill( common::message::transaction::resource::rollback::Reply& message);

      } // detail
      
      template< typename M>
      M message() 
      {
         M result;
         detail::fill( result);
         return result;
      }

      namespace representation
      {
         namespace detail
         {
            constexpr std::string_view base64( gateway::message::domain::connect::Request&&){
               return R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAAAUAAAAAAAAD7AAAAAAAAAPrAAAAAAAAA+oAAAAAAAAD6QAAAAAAAAPo)";
            }
            constexpr std::string_view base64( gateway::message::domain::connect::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAA+g=)";
            }
            constexpr std::string_view base64( casual::domain::message::discovery::Request&&){
               return R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4YAAAAAAAAACGRvbWFpbiBBAAAAAAAAAAMAAAAAAAAACHNlcnZpY2UxAAAAAAAAAAhzZXJ2aWNlMgAAAAAAAAAIc2VydmljZTMAAAAAAAAAAwAAAAAAAAAGcXVldWUxAAAAAAAAAAZxdWV1ZTIAAAAAAAAABnF1ZXVlMw==)";
            }
            constexpr std::string_view base64( casual::domain::message::discovery::v1_3::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YOL2t8N/c0oJgqCrFYGyH6UAAAAAAAAACGRvbWFpbiBCAAAAAAAAAAEAAAAAAAAACHNlcnZpY2UxAAAAAAAAAAdleGFtcGxlAAEAAAAU9GsEAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAABnF1ZXVlMQAAAAAAAAAK)";
            }
            constexpr std::string_view base64( casual::domain::message::discovery::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YOL2t8N/c0oJgqCrFYGyH6UAAAAAAAAACGRvbWFpbiBCAAAAAAAAAAEAAAAAAAAACHNlcnZpY2UxAAAAAAAAAAdleGFtcGxlAAEAAAAU9GsEAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAABnF1ZXVlMQAAAAAAAAAKAAAAAAA9CQABAA==)";
            }
            constexpr std::string_view base64( common::message::service::call::v1_2::callee::Request&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAIc2VydmljZTEAAAAJx2UkAAAAAAAAAAAOcGFyZW50LXNlcnZpY2UAAAAAAAAAKgAAAAAAAAAQAAAAAAAAABBbbBv28ktIDb283vVMOghRW2wb9vJLSA29vN71TDoIUgAAAAAAAAAEAAAAAAAAAAguYmluYXJ5LwAAAAAAAACAgIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6vsLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f4OHi4+Tl5ufo6err7O3u7/Dx8vP09fb3+Pn6+/z9/v8=)";
            }
            constexpr std::string_view base64( common::message::service::call::callee::Request&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAIc2VydmljZTEAAAAJx2UkAICBgoOEhYaHAAAAAAAAAA5wYXJlbnQtc2VydmljZQAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAAAAAAAQAAAAAAAAACC5iaW5hcnkvAAAAAAAAAICAgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==)";
            }
            constexpr std::string_view base64( common::message::service::call::v1_2::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAsAAAAAAAAAKgAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAAAAAAAAILmJpbmFyeS8AAAAAAAAAgICBgoOEhYaHiImKi4yNjo+QkZKTlJWWl5iZmpucnZ6foKGio6SlpqeoqaqrrK2ur7CxsrO0tba3uLm6u7y9vr/AwcLDxMXGx8jJysvMzc7P0NHS09TV1tfY2drb3N3e3+Dh4uPk5ebn6Onq6+zt7u/w8fLz9PX29/j5+vv8/f7/)";
            }
            constexpr std::string_view base64(common::message::service::call::Reply &&){
               return  R"(cHPL9BRESkGHswCG8UP8YAAAAAsAAAAAAAAAKgAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAAAAAAAAILmJpbmFyeS8AAAAAAAAAgICBgoOEhYaHiImKi4yNjo+QkZKTlJWWl5iZmpucnZ6foKGio6SlpqeoqaqrrK2ur7CxsrO0tba3uLm6u7y9vr/AwcLDxMXGx8jJysvMzc7P0NHS09TV1tfY2drb3N3e3+Dh4uPk5ebn6Onq6+zt7u/w8fLz9PX29/j5+vv8/f7/)";
            }
            constexpr std::string_view base64( common::message::conversation::connect::callee::Request&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAIc2VydmljZTEAAAAJx2UkAICBgoOEhYaHAAAAAAAAAA5wYXJlbnQtc2VydmljZQAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAAAAAAAAACC5iaW5hcnkvAAAAAAAAAICAgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==)";
            }
            constexpr std::string_view base64( common::message::conversation::connect::v1_2::callee::Request&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAIc2VydmljZTEAAAAJx2UkAAAAAAAAAAAOcGFyZW50LXNlcnZpY2UAAAAAAAAAKgAAAAAAAAAQAAAAAAAAABBbbBv28ktIDb283vVMOghRW2wb9vJLSA29vN71TDoIUgAAAAAAAAAAAAguYmluYXJ5LwAAAAAAAACAgIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6vsLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f4OHi4+Tl5ufo6err7O3u7/Dx8vP09fb3+Pn6+/z9/v8=)";
            }
            constexpr std::string_view base64( common::message::transaction::resource::prepare::Request&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAAAAAAA)";
            }
            constexpr std::string_view base64(common::message::transaction::resource::commit::Request &&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAAAAAAA)";
            }
            constexpr std::string_view base64( common::message::transaction::resource::commit::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAA=)";
            }
            constexpr std::string_view base64( common::message::transaction::resource::rollback::Request&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAAAAAAA)";
            }
            constexpr std::string_view base64( common::message::transaction::resource::rollback::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAqAAAAAAAAABAAAAAAAAAAEFtsG/byS0gNvbze9Uw6CFFbbBv28ktIDb283vVMOghSAAAAKgAAAAA=)";
            }
            constexpr std::string_view base64( queue::ipc::message::group::enqueue::Request&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAGcXVldWVBAAAAAAAAACoAAAAAAAAAEAAAAAAAAAAQW2wb9vJLSA29vN71TDoIUVtsG/byS0gNvbze9Uw6CFLm/Z/PhqxH9KUlL1l+JfxqAAAAAAAAABVwcm9wZXJ0eSAxOnByb3BlcnR5IDIAAAAAAAAABnF1ZXVlQhWlY3jTqfCgAAAAAAAAAAguYmluYXJ5LwAAAAAAAACAgIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6vsLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f4OHi4+Tl5ufo6err7O3u7/Dx8vP09fb3+Pn6+/z9/v8=)";
            }
            constexpr std::string_view base64( queue::ipc::message::group::enqueue::v1_2::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YDFdrMYYLkwSv5h376kky4c=)";
            }
            constexpr std::string_view base64( queue::ipc::message::group::enqueue::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YByY2UvSmkBhnYpzABnciR8AAAAe)";
            }
            constexpr std::string_view base64( queue::ipc::message::group::dequeue::Request&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAAGcXVldWVBAAAAAAAAACoAAAAAAAAAEAAAAAAAAAAQW2wb9vJLSA29vN71TDoIUVtsG/byS0gNvbze9Uw6CFIAAAAAAAAAFXByb3BlcnR5IDE6cHJvcGVydHkgMjFdrMYYLkwSv5h376kky4cA)";
            }
            constexpr std::string_view base64( queue::ipc::message::group::dequeue::v1_2::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YAAAAAAAAAABUy+LbBV2Tcqf6CowAt5XngAAAAAAAAAVcHJvcGVydHkgMTpwcm9wZXJ0eSAyAAAAAAAAAAZxdWV1ZUIVpWN406nwoAAAAAAAAAAGLmpzb24vAAAAAAAAAAJ7fQAAAAAAAAABFaVjeNOp8KA=)";
            }
            constexpr std::string_view base64( queue::ipc::message::group::dequeue::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YAFTL4tsFXZNyp/oKjAC3leeAAAAAAAAABVwcm9wZXJ0eSAxOnByb3BlcnR5IDIAAAAAAAAABnF1ZXVlQhWlY3jTqfCgAAAAAAAAAAYuanNvbi8AAAAAAAAAAnt9AAAAAAAAAAEVpWN406nwoAAAABQ=)";
            }
            constexpr std::string_view base64( gateway::message::domain::disconnect::Request&&){
               return  R"(cHPL9BRESkGHswCG8UP8YA==)";
            }
            constexpr std::string_view base64( gateway::message::domain::disconnect::Reply&&){
               return R"(cHPL9BRESkGHswCG8UP8YA==)";
            }
         } // detail

         template< typename M>
         constexpr std::string_view base64()
         {
            return detail::base64( M{});
         }
         
      } // representation


   } // gateway::documentation::protocol::example
} // casual
