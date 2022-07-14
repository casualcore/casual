//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/event/dispatch.h"
#include "common/communication/ipc/send.h"

namespace casual
{
   namespace common
   {
      TEST( common_event_dispatch, collection_default_ctor)
      {
         common::unittest::Trace trace;

         event::dispatch::Collection<
            message::event::Error,
            message::event::process::Spawn,
            message::event::process::Exit> collection;

         EXPECT_TRUE( ! collection.active< message::event::process::Exit>());
      }

      TEST( common_event_dispatch, collection_subscribe_error)
      {
         common::unittest::Trace trace;

         event::dispatch::Collection<
            message::event::Error,
            message::event::process::Spawn,
            message::event::process::Exit> collection;

         {
            message::event::subscription::Begin begin;
            begin.process = process::handle();
            begin.types.push_back( message::event::Error::type());

            collection.subscription( begin);
         }

         EXPECT_TRUE( collection.active< message::event::Error>()) << trace.compose( CASUAL_NAMED_VALUE( collection));
         EXPECT_TRUE( ! collection.active< message::event::process::Spawn>());
         EXPECT_TRUE( ! collection.active< message::event::process::Exit>());
      }

      TEST( common_event_dispatch, collection_subscribe_error_spawn_exit)
      {
         common::unittest::Trace trace;

         event::dispatch::Collection<
            message::event::Error,
            message::event::process::Spawn,
            message::event::process::Exit> collection;

         {
            message::event::subscription::Begin begin;
            begin.process = process::handle();
            begin.types = {
                  message::event::Error::type(),
                  message::event::process::Exit::type(),
                  message::event::process::Spawn::type()};

            collection.subscription( begin);
         }

         EXPECT_TRUE( collection.active< message::event::Error>()) << trace.compose( CASUAL_NAMED_VALUE( collection));
         EXPECT_TRUE( collection.active< message::event::process::Spawn>());
         EXPECT_TRUE( collection.active< message::event::process::Exit>());
      }

      TEST( common_event_dispatch, collection_unsubscribe_error)
      {
         common::unittest::Trace trace;

         event::dispatch::Collection<
            message::event::Error,
            message::event::process::Spawn,
            message::event::process::Exit> collection;

         {
            message::event::subscription::Begin begin;
            begin.process = process::handle();
            begin.types.push_back( message::event::Error::type());

            collection.subscription( begin);
         }

         {
            message::event::subscription::End end;
            end.process = process::handle();

            collection.subscription( end);
         }

         EXPECT_TRUE( ! collection.active< message::event::Error>()) << trace.compose( CASUAL_NAMED_VALUE( collection), " ", CASUAL_NAMED_VALUE( process::handle()));
         EXPECT_TRUE( ! collection.active< message::event::process::Spawn>());
         EXPECT_TRUE( ! collection.active< message::event::process::Exit>());
      }

      TEST( common_event_dispatch, collection_dispatch)
      {
         common::unittest::Trace trace;

         struct
         {
            communication::select::Directive directive;
            communication::ipc::send::Coordinator multiplex{ directive};
         } state;

         event::dispatch::Collection<
            message::event::Error,
            message::event::process::Spawn,
            message::event::process::Exit> events;

         // we register our self to process exit event.
         {
            message::event::subscription::Begin begin{ process::handle()};
            begin.types = { message::event::process::Exit::type()};
            events.subscription( begin);
         }

         // dispatch process exit event (to our self)
         {
            message::event::process::Exit exit;
            exit.state.pid = process::id();
            exit.state.status = 42;
            exit.state.reason = decltype( exit.state.reason)::core;

            events( state.multiplex, exit);
         }
         
         // make sure the whole event reaches us.
         while( ! state.multiplex.send())
            communication::ipc::inbound::device().flush();

         auto event = communication::ipc::receive< message::event::process::Exit>();

         EXPECT_TRUE( event.state.pid == process::id());
         EXPECT_TRUE( event.state.status == 42);
         EXPECT_TRUE( event.state.reason == decltype( event.state.reason)::core);
      }

   } // common

} // casual
