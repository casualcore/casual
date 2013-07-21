//!
//! test_message_dispatch.cpp
//!
//! Created on: Dec 2, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>




#include "common/message.h"
#include "common/message_dispatch.h"



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

               typedef message::server::Disconnect message_type;

               void dispatch( message_type& message)
               {

               }

            };


         }

      }

      TEST( casual_common_message_dispatch, add)
      {
         message::dispatch::Handler handler;

         EXPECT_TRUE( handler.size() == 0);

         handler.add< local::TestHandler>();

         EXPECT_TRUE( handler.size() == 1);
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
               message::dispatch::Handler handler;

               handler.add< local::TestHandler>();

               marshal::output::Binary output;
               local::TestHandler::message_type message;
               output << message;

               marshal::input::Binary input( std::move( output));
               // m_messageType is private, but we're good friends now...
               input.m_messageType = local::TestHandler::message_type::message_type;

               EXPECT_TRUE( handler.dispatch( input));
            }

            TEST( casual_common_message_dispatch, dispatch__gives_no_found_handler)
            {
               message::dispatch::Handler handler;

               handler.add< local::TestHandler>();

               marshal::output::Binary output;
               message::service::ACK message;
               output << message;

               marshal::input::Binary input( std::move( output));

               //
               // We have not handler for this message-type.
               //
               input.m_messageType = message.message_type;

               EXPECT_FALSE( handler.dispatch( input));
            }

         }
      }



   }
}

