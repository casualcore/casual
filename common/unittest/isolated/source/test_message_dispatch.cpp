//!
//! test_message_dispatch.cpp
//!
//! Created on: Dec 2, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>




#include "common/message/server.h"
#include "common/message/dispatch.h"



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

               void dispatch( message_type& message)
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

               EXPECT_TRUE( handler.dispatch( complete));
            }

            TEST( casual_common_message_dispatch, dispatch__gives_no_found_handler)
            {
               message::dispatch::Handler handler{ local::TestHandler()};

               message::service::ACK message;
               ipc::message::Complete complete = marshal::complete( message);

               //
               // We have not handler for this message-type.
               //

               EXPECT_FALSE( handler.dispatch( complete));
            }

         }
      }



   }
}

