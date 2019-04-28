//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/event/listen.h"
#include "common/unittest/eventually/send.h"

#include "common/exception/casual.h"


namespace casual
{
   namespace common
   {
      TEST( common_event, empty_dispatch__expect_no_registration)
      {
         unittest::Trace trace;

         // Make sure we get a shutdown
         {
            unittest::eventually::send( communication::ipc::inbound::ipc(), message::shutdown::Request{});
         }

         EXPECT_THROW({
            event::listen();
         }, exception::casual::Shutdown);
      }

      TEST( common_event, process_exit_event___expect_registration__event_dispatch)
      {
         unittest::Trace trace;

         {
            // send the event, premature...
            message::event::process::Exit event;
            event.state.pid = strong::process::id{ 42};
            event.state.reason = process::lifetime::Exit::Reason::core;

            unittest::eventually::send( communication::ipc::inbound::ipc(), event);

         }

         // Make sure we get a shutdown
         {
            unittest::eventually::send( communication::ipc::inbound::ipc(), message::shutdown::Request{});
         }

         // listen to the event
         {
            EXPECT_THROW({
               event::no::subscription::listen( communication::ipc::inbound::device(), []( message::event::process::Exit& m){
                  EXPECT_TRUE( m.state.pid == strong::process::id{ 42});
                  EXPECT_TRUE( m.state.reason == process::lifetime::Exit::Reason::core);
               });
            }, exception::casual::Shutdown);
         }
      }

   } // common
} // casual
