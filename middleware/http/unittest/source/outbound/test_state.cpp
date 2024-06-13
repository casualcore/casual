//!
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "http/outbound/state.h"



namespace casual
{
   namespace http
   {
      namespace outbound
      {
         TEST( http_outbound_state, empty)
         {
            common::unittest::Trace trace;

            State state;

            EXPECT_TRUE( state.pending.requests.empty());
         }

         TEST( http_outbound_state_request, empty)
         {
            common::unittest::Trace trace;

            state::pending::Request request;

            EXPECT_TRUE( request.easy());
            EXPECT_TRUE( request.state().header.reply.empty());
            EXPECT_TRUE( request.state().header.request.native() == nullptr);
         }

         TEST( http_outbound_state_request, add_request_headers)
         {
            common::unittest::Trace trace;

            {
               state::pending::Request request;
               request.state().header.request.add( common::service::header::Fields{{
                  common::service::header::Field{ "key-1", "value-1"},
                  common::service::header::Field{ "key-2", "value-2"},
                  common::service::header::Field{ "key-3", "value-3"},
               }});

               EXPECT_TRUE( request.easy());
               EXPECT_TRUE( request.state().header.reply.empty());
               EXPECT_TRUE( request.state().header.request.native() != nullptr);
            }

            {
               state::pending::Request request;

               EXPECT_TRUE( request.easy());
               EXPECT_TRUE( request.state().header.request.native() == nullptr);
            }
         }

      } // outbound
   } // http
} // casual
