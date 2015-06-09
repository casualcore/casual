//!
//! test_message_dispatch.cpp
//!
//! Created on: Dec 2, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>




#include "common/message/server.h"
#include "common/message/dispatch.h"
#include "common/message/transaction.h"



namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            struct TestHandler
            {
               TestHandler() = default;
               //TestHandler( TestHandler&&) = default;

               typedef message::shutdown::Request message_type;

               void operator () ( message_type message)
               {

               }

            };


         }

      }

      TEST( casual_common_message_dispatch, construct)
      {
         message::dispatch::Handler handler{ local::TestHandler()};

         EXPECT_TRUE( handler.size() == 1);

         auto types = handler.types();

         ASSERT_TRUE( types.size() == 1);
         EXPECT_TRUE( types.at( 0) == message::shutdown::Request::message_type);

      }

      //
      // We have to be in the same ns as marshal::input so the friend-declaration
      // for input::Binary kicks in.
      //
      namespace marshal
      {
         namespace input
         {

            TEST( casual_common_message_dispatch, dispatch__gives_correct_dispatch)
            {
               message::dispatch::Handler handler{ local::TestHandler()};

               local::TestHandler::message_type message;
               ipc::message::Complete complete = marshal::complete( message);

               EXPECT_TRUE( handler( complete));
            }

            TEST( casual_common_message_dispatch, dispatch__gives_no_found_handler)
            {
               message::dispatch::Handler handler{ local::TestHandler()};

               message::service::call::ACK message;
               ipc::message::Complete complete = marshal::complete( message);

               //
               // We have not handler for this message-type.
               //

               EXPECT_FALSE( handler( complete));
            }

         }
      }


      namespace message
      {

         TEST( casual_common_message_reverse, transaction_resource_rollback_Request__gives__transaction_resource_rollback_Reply)
         {
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

