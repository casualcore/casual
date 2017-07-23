//!
//! casual 
//!

#include "common/unittest.h"

#include "common/event/listen.h"
#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"

#include "common/exception/casual.h"


namespace casual
{
   namespace common
   {
      TEST( common_event, empty_dispatch__expect_no_registration)
      {
         unittest::Trace trace;

         //
         // Make sure we get a shutdown
         //
         {
            mockup::ipc::eventually::send( communication::ipc::inbound::id(), message::shutdown::Request{});
         }

         EXPECT_THROW({
            event::listen();
         }, exception::casual::Shutdown);
      }

      TEST( common_event, process_exit_event___expect_registration__event_dispatch)
      {
         unittest::Trace trace;


         mockup::domain::Manager manager;

         {
            //
            // send the event, premature...
            //
            message::event::process::Exit event;
            event.state.pid = 42;
            event.state.reason = process::lifetime::Exit::Reason::core;

            mockup::ipc::eventually::send( communication::ipc::inbound::id(), event);

         }

         //
         // Make sure we get a shutdown
         //
         {
            mockup::ipc::eventually::send( communication::ipc::inbound::id(), message::shutdown::Request{});
         }

         //
         // listen to the event
         //
         {
            EXPECT_THROW({
               event::listen( []( message::event::process::Exit& m){
                  EXPECT_TRUE( m.state.pid == 42);
                  EXPECT_TRUE( m.state.reason == process::lifetime::Exit::Reason::core);
               });
            }, exception::casual::Shutdown);
         }
      }

   } // common
} // casual
