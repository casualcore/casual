//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <gtest/gtest.h>

#include "http/outbound/state.h"
#include "http/outbound/request.h"

#include "common/unittest.h"
#include "common/message/service.h"
#include "common/process.h"
#include "common/strong/id.h"
#include "common/transaction/id.h"

namespace casual::http::outbound
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


   TEST( http_outbound_request, prepare_request)
   {
      common::unittest::Trace trace;

      common::message::service::call::callee::Request call;
      call.process = common::process::handle();
      call.correlation = common::strong::correlation::id::generate();
      call.execution = common::strong::execution::id::generate();
      call.service.name = "some-service";
      call.service.requested = "some-route";
      call.parent = "some-other-service";
      call.trid = common::transaction::id::create();

      auto expected = call;
      auto request = request::prepare( local::node(), std::move( call));

      EXPECT_TRUE( request.state().destination == expected.process) << CASUAL_NAMED_VALUE( request.state().destination);
      EXPECT_TRUE( request.state().correlation == expected.correlation) << request.state().correlation;
      EXPECT_TRUE( request.state().execution == expected.execution) << request.state().execution;
      EXPECT_TRUE( request.state().service == expected.service.logical_name()) << request.state().service;
      EXPECT_TRUE( request.state().parent == expected.parent) << request.state().parent;
      EXPECT_TRUE( request.state().trid == expected.trid) << request.state().trid;
      EXPECT_TRUE( request.state().url == local::node().url) << request.state().url;
   }

} // casual::http::outbound
