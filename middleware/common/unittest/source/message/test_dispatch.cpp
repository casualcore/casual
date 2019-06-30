//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <common/unittest.h>




#include "common/message/service.h"
#include "common/message/server.h"
#include "common/message/dispatch.h"
#include "common/message/transaction.h"
#include "common/communication/ipc.h"

#include "common/serialize/native/complete.h"


#include <functional>



namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            using dispatch_type = communication::ipc::dispatch::Handler;

            struct TestHandler
            {
               using message_type = message::shutdown::Request;

               void operator () ( message_type message)
               {

               }

            };

            struct TestMember
            {
               void handle( message::server::ping::Request& message)
               {

               }

            };

         }

      }

      TEST( casual_common_message_dispatch, construct)
      {
         local::dispatch_type handler{ local::TestHandler()};

         EXPECT_TRUE( handler.size() == 1);

         auto types = handler.types();

         ASSERT_TRUE( types.size() == 1);
         EXPECT_TRUE( types.at( 0) == message::shutdown::Request::type());

      }

      /*
       * Does not work. need to work on traits::function...
      TEST( casual_common_message_dispatch, construct_with_member_function)
      {
         local::TestMember holder;

         local::dispatch_type handler{ std::bind( &local::TestMember::handle, &holder, std::placeholders::_1)};

         EXPECT_TRUE( handler.size() == 1);

         auto types = handler.types();

         ASSERT_TRUE( types.size() == 1);
         EXPECT_TRUE( types.at( 0) == message::server::ping::Request::message_type);
      }
      */

      // We have to be in the same ns as marshal::input so the friend-declaration
      // for input::Binary kicks in.
      namespace marshal
      {
         namespace input
         {

            TEST( casual_common_message_dispatch, dispatch__gives_correct_dispatch)
            {
               common::unittest::Trace trace;

               local::dispatch_type handler{ local::TestHandler()};

               local::TestHandler::message_type message;
               auto complete = serialize::native::complete( message);

               EXPECT_TRUE( handler( complete));
            }

            TEST( casual_common_message_dispatch, dispatch__gives_no_found_handler)
            {
               common::unittest::Trace trace;

               local::dispatch_type handler{ local::TestHandler()};

               message::service::call::ACK message;
               auto complete = serialize::native::complete( message);

               // We have not handler for this message-type.
               EXPECT_FALSE( handler( complete));
            }

         }
      }


      namespace message
      {

         TEST( casual_common_message_reverse, transaction_resource_rollback_Request__gives__transaction_resource_rollback_Reply)
         {
            common::unittest::Trace trace;

            message::transaction::resource::rollback::Request request;
            request.correlation = uuid::make();
            request.execution = uuid::make();

            auto reply = message::reverse::type( request);

            auto is_same = std::is_same< message::transaction::resource::rollback::Reply, decltype( reply)>::value;
            EXPECT_TRUE( is_same);
            EXPECT_TRUE( request.correlation == reply.correlation);
            EXPECT_TRUE( request.execution == reply.execution);
         }


      } // message

   } // common
} // casual

