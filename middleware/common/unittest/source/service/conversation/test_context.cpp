//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/service/conversation/context.h"

#include "common/mockup/domain.h"

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace conversation
         {
            namespace local
            {
               namespace
               {
                  struct Domain
                  {
                     Domain() : server{
                        mockup::domain::echo::create::service( "echo.conversation"),
                     }
                     {}

                     mockup::domain::Manager manager;
                     mockup::domain::service::Manager service;
                     mockup::domain::transaction::Manager tm;
                     mockup::domain::echo::Server server;
                  };

                  buffer::Payload payload( platform::size::type size = 128)
                  {
                     buffer::Payload result{ "string/", size};

                     unittest::random::range( result.memory);

                     return result;
                  }

               } // <unnamed>
            } // local

            TEST( common_service_conversation, empty_mockup_domain)
            {
               common::unittest::Trace trace;

               EXPECT_NO_THROW({
                  local::Domain domain;
               });
            }

            TEST( common_service_conversation, connect_no_flags__expect_throw)
            {
               common::unittest::Trace trace;

               local::Domain domain;

               auto payload = local::payload();

               EXPECT_THROW({
                  conversation::Context::instance().connect( "echo.conversation", payload, {});
               }, exception::xatmi::invalid::Argument);
            }

            TEST( common_service_conversation, connect_send_and_receive_flags__expect_throw)
            {
               common::unittest::Trace trace;

               local::Domain domain;

               auto payload = local::payload();

               EXPECT_THROW({
                  conversation::Context::instance().connect(
                        "echo.conversation", payload, { connect::Flag::send_only, connect::Flag::receive_only});
               }, exception::xatmi::invalid::Argument);
            }

            TEST( common_service_conversation, connect)
            {
               common::unittest::Trace trace;

               local::Domain domain;

               auto payload = local::payload();

               auto descriptor = conversation::Context::instance().connect( "echo.conversation", payload, { connect::Flag::send_only});

               EXPECT_TRUE( descriptor != -1);
            }

            TEST( common_service_conversation, connect_receivce)
            {
               common::unittest::Trace trace;

               local::Domain domain;

               auto payload = local::payload();

               auto descriptor = conversation::Context::instance().connect( "echo.conversation", payload, { connect::Flag::receive_only});

               auto result = conversation::Context::instance().receive( descriptor, {});


               EXPECT_TRUE( result.buffer.type == payload.type);
               EXPECT_TRUE( result.buffer.memory == payload.memory);
            }

            TEST( common_service_conversation, connect_send_only__send_receive)
            {
               common::unittest::Trace trace;

               local::Domain domain;

               common::buffer::Payload null{ nullptr};

               const auto descriptor = conversation::Context::instance().connect( "echo.conversation", null, { connect::Flag::send_only});


               auto payload = local::payload();

               auto event = conversation::Context::instance().send( descriptor, payload, { send::Flag::receive_only});
               EXPECT_TRUE( event.empty());

               auto result = conversation::Context::instance().receive( descriptor, {});


               EXPECT_TRUE( result.buffer.type == payload.type);
               EXPECT_TRUE( result.buffer.memory == payload.memory);
            }






            TEST( common_service_conversation, connect_receive_only___disconnect)
            {
               common::unittest::Trace trace;

               local::Domain domain;

               auto payload = local::payload();

               auto descriptor = conversation::Context::instance().connect( "echo.conversation", payload, { connect::Flag::receive_only});

               EXPECT_NO_THROW({
                  conversation::Context::instance().disconnect( descriptor);
               });
            }

            TEST( common_service_conversation, connect_send_only__disconnect)
            {
               common::unittest::Trace trace;

               local::Domain domain;

               auto payload = local::payload();

               auto descriptor = conversation::Context::instance().connect( "echo.conversation", payload, { connect::Flag::send_only});

               EXPECT_NO_THROW({
                  conversation::Context::instance().disconnect( descriptor);
               });
            }

         } // conversation

      } // service

   } // common

} // casual
