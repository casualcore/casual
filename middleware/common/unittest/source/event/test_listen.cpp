//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/event/listen.h"
#include "common/unittest/eventually/send.h"

#include "common/code/casual.h"


namespace casual
{
   namespace common
   {
      TEST( common_event, empty_dispatch__empty_condition__expect_no_registration)
      {
         unittest::Trace trace;

         // Make sure we get a shutdown
         {
            unittest::eventually::send( communication::ipc::inbound::ipc(), message::shutdown::Request{});
         }

         EXPECT_CODE({
            event::listen( event::condition::compose());
         }, code::casual::shutdown);
      }

      TEST( common_event, process_exit_event___expect_registration__event_dispatch)
      {
         unittest::Trace trace;

         bool done = false;

         auto condition = event::condition::compose( 
            event::condition::prelude( []()
            {
               // send the event, premature...
               message::event::process::Exit event;
               event.state.pid = strong::process::id{ 42};
               event.state.reason = decltype( event.state.reason)::core;

               unittest::eventually::send( communication::ipc::inbound::ipc(), event);
            }),
            event::condition::done( [&done](){ return done;})
         );

         EXPECT_NO_THROW({
            event::only::unsubscribe::listen( condition, 
            [&done]( message::event::process::Exit& m)
            {
               done = true;
               EXPECT_TRUE( m.state.pid == strong::process::id{ 42});
               EXPECT_TRUE( m.state.reason == decltype( m.state.reason)::core);
            });
         });

      }

   } // common
} // casual
