//!
//! casual 
//!

#include "common/unittest.h"

#include "common/event/dispatch.h"

namespace casual
{
   namespace common
   {
      TEST( common_event_dispatch, collection_default_ctor)
      {
         common::unittest::Trace trace;

         event::dispatch::Collection<
            message::event::domain::Error,
            message::event::process::Spawn,
            message::event::process::Exit> collection;

         EXPECT_TRUE( ! collection.active< message::event::process::Exit>());
      }

      TEST( common_event_dispatch, collection_create_event)
      {
         common::unittest::Trace trace;

         event::dispatch::Collection<
            message::event::domain::Error,
            message::event::process::Spawn,
            message::event::process::Exit> collection;

         message::event::process::Exit event;

         auto pending = collection( event);

         // we have no 'subscription', hence no targets, hence pending is interpret as sent.
         EXPECT_TRUE( pending.sent()) << "pending: " << pending;
      }

      TEST( common_event_dispatch, collection_subscribe_error)
      {
         common::unittest::Trace trace;

         event::dispatch::Collection<
            message::event::domain::Error,
            message::event::process::Spawn,
            message::event::process::Exit> collection;

         {
            message::event::subscription::Begin begin;
            begin.process = process::handle();
            begin.types.push_back( message::event::domain::Error::type());

            collection.subscription( begin);
         }

         EXPECT_TRUE( collection.active< message::event::domain::Error>()) << "collection: " << collection;
         EXPECT_TRUE( ! collection.active< message::event::process::Spawn>());
         EXPECT_TRUE( ! collection.active< message::event::process::Exit>());
      }

      TEST( common_event_dispatch, collection_subscribe_error_spawn_exit)
      {
         common::unittest::Trace trace;

         event::dispatch::Collection<
            message::event::domain::Error,
            message::event::process::Spawn,
            message::event::process::Exit> collection;

         {
            message::event::subscription::Begin begin;
            begin.process = process::handle();
            begin.types = {
                  message::event::domain::Error::type(),
                  message::event::process::Exit::type(),
                  message::event::process::Spawn::type()};

            collection.subscription( begin);
         }

         EXPECT_TRUE( collection.active< message::event::domain::Error>()) << "collection: " << collection;
         EXPECT_TRUE( collection.active< message::event::process::Spawn>());
         EXPECT_TRUE( collection.active< message::event::process::Exit>());
      }

      TEST( common_event_dispatch, collection_unsubscribe_error)
      {
         common::unittest::Trace trace;

         event::dispatch::Collection<
            message::event::domain::Error,
            message::event::process::Spawn,
            message::event::process::Exit> collection;

         {
            message::event::subscription::Begin begin;
            begin.process = process::handle();
            begin.types.push_back( message::event::domain::Error::type());

            collection.subscription( begin);
         }

         {
            message::event::subscription::End end;
            end.process = process::handle();

            collection.subscription( end);
         }

         EXPECT_TRUE( ! collection.active< message::event::domain::Error>()) << "collection: " << collection << " process: " << process::handle();
         EXPECT_TRUE( ! collection.active< message::event::process::Spawn>());
         EXPECT_TRUE( ! collection.active< message::event::process::Exit>());
      }


   } // common

} // casual
