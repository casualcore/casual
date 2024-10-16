//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <common/unittest.h>

#include "common/signal.h"
#include "common/process.h"

#include "common/message/service.h"
#include "common/message/server.h"
#include "common/message/dispatch.h"
#include "common/message/transaction.h"
#include "common/communication/ipc.h"
#include "common/code/signal.h"
#include "common/exception/compose.h"

#include "common/serialize/native/complete.h"


#include <functional>
#include <system_error>
#include <optional>



namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            using dispatch_type = decltype( message::dispatch::handler( communication::ipc::inbound::device()));

            struct TestHandler
            {
               void operator () ( message::shutdown::Request message)
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

      TEST( common_message_dispatch, construct)
      {
         local::dispatch_type handler{ []( message::shutdown::Request&){}};

         EXPECT_TRUE( handler.size() == 1);

         auto types = handler.types();

         ASSERT_TRUE( types.size() == 1);
         EXPECT_TRUE( types.at( 0) == message::shutdown::Request::type());
      }

      TEST( common_message_dispatch, extra_dispatch_arguments)
      {
         using complete_type = communication::ipc::message::Complete;
         using handler_type = message::dispatch::basic_handler< complete_type, const std::string&, long>;

         struct
         {
            std::string text;
            long value{};
         } state;
         
         handler_type handler{ 
            []( message::shutdown::Request&){}, // no extra arguments
            [ &state]( message::flush::IPC&&, const std::string& text, long value)
            {
               state.text = text;
               state.value = value;
            },
            [ &state]( message::event::Idle&&, std::string text, const long& value)
            {
               state.text = text;
               state.value = value;
            }};

         EXPECT_TRUE( handler.size() == 3);

         handler( serialize::native::complete< complete_type>( message::shutdown::Request{}), "poop", 4);
         // expect the extra dispatch arguments to be discarded since the handler
         // for shutdown::Request does not have any extra arguments in its signature
         EXPECT_TRUE( state.text.empty());
         EXPECT_TRUE( state.value == 0);

         handler( serialize::native::complete< complete_type>( message::flush::IPC{}), "text", 42);
         EXPECT_TRUE( state.text == "text");
         EXPECT_TRUE( state.value == 42);

         handler( serialize::native::complete< complete_type>( message::event::Idle{}), "idle", 777);
         EXPECT_TRUE( state.text == "idle");
         EXPECT_TRUE( state.value == 777);

      }

      TEST( common_message_dispatch, condition)
      {
         bool idle = false;
         auto condition = message::dispatch::condition::compose( message::dispatch::condition::idle( [&idle](){ idle = true;}));

         message::dispatch::condition::detail::invoke< message::dispatch::condition::detail::tag::idle>( condition);
         EXPECT_TRUE( idle);
      }

      TEST( common_message_dispatch, handle_error)
      {
         signal::send( process::id(), code::signal::terminate);

         try
         {
            throw exception::compose( code::casual::interrupted);
         }
         catch( ...)
         {
            std::optional< std::system_error> error;
            EXPECT_NO_THROW( error = message::dispatch::condition::detail::handle::error());

            EXPECT_TRUE( error && error->code() == code::signal::terminate);
         }
      }


      // We have to be in the same ns as marshal::input so the friend-declaration
      // for input::Binary kicks in.
      namespace marshal
      {
         namespace input
         {

            TEST( common_message_dispatch, dispatch__gives_correct_dispatch)
            {
               common::unittest::Trace trace;

               local::dispatch_type handler{ local::TestHandler()};

               message::shutdown::Request message;
               auto complete = serialize::native::complete< communication::ipc::message::Complete>( message);

               EXPECT_TRUE( handler( complete));
            }

            TEST( common_message_dispatch, dispatch__gives_no_found_handler)
            {
               common::unittest::Trace trace;

               local::dispatch_type handler{ local::TestHandler()};

               message::service::call::ACK message;
               auto complete = serialize::native::complete< communication::ipc::message::Complete>( message);

               // We have not handler for this message-type.
               EXPECT_FALSE( handler( complete));
            }

         }
      }


      namespace message
      {

         TEST( common_message_reverse, transaction_resource_rollback_Request__gives__transaction_resource_rollback_Reply)
         {
            common::unittest::Trace trace;

            message::transaction::resource::rollback::Request request;
            request.correlation = strong::correlation::id::generate();
            request.execution = strong::execution::id::generate();

            auto reply = message::reverse::type( request);

            auto is_same = std::is_same< message::transaction::resource::rollback::Reply, decltype( reply)>::value;
            EXPECT_TRUE( is_same);
            EXPECT_TRUE( request.correlation == reply.correlation);
            EXPECT_TRUE( request.execution == reply.execution);
         }


      } // message

   } // common
} // casual

