//!
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>


#include "http/outbound/state.h"
#include "http/outbound/request.h"

#include "common/unittest.h"



namespace casual
{
   namespace http
   {
      namespace outbound
      {
         namespace local
         {
            namespace
            {
               const state::Node& node()
               {
                  static const auto singleton = [](){
                     state::Node value;
                     value.discard_transaction = true;
                     value.url = "http://casual.laz.se/some/service";
                     value.headers = std::make_shared< const common::service::header::Fields>();
                     return value;
                  }();

                  return singleton;
               }

            } // <unnamed>
         } // local

         TEST( http_outbound_transcode, empty_buffer)
         {
            common::unittest::Trace trace;

            common::message::service::call::callee::Request call;
            call.buffer.type = common::buffer::type::binary;

            auto request = request::prepare( local::node(), std::move( call));

            auto payload = request::receive::payload( std::move( request));

            EXPECT_TRUE( payload.data.empty());
         }

         TEST( http_outbound_transcode, null_buffer)
         {
            common::unittest::Trace trace;

            common::message::service::call::callee::Request call;
            call.buffer = common::buffer::Payload{ nullptr};

            auto request = request::prepare( local::node(), std::move( call));

            auto payload = request::receive::payload( std::move( request));

            EXPECT_TRUE( payload.null());
            EXPECT_TRUE( payload.data.empty());
         }

         TEST( http_outbound_transcode, binary_buffer)
         {
            common::unittest::Trace trace;

            const auto payload = common::unittest::random::binary( 1024);

            common::message::service::call::callee::Request call;
            call.buffer.type = common::buffer::type::binary;
            call.buffer.data = payload;

            auto request = request::prepare( local::node(), std::move( call));

            auto buffer = request::receive::payload( std::move( request));

            EXPECT_TRUE( buffer.data == payload);
         }
      } // outbound
   } // http
} // casual
