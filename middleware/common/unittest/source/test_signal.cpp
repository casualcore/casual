//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/signal.h"
#include "common/signal/timer.h"
#include "common/process.h"

#include "common/code/signal.h"


namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            auto expect_signal_raised = []( auto&& action, auto signal)
            {
               try
               {
                  action();
               }
               catch( ...)
               {
                  auto error = exception::capture();
                  EXPECT_TRUE( error.code() == signal)  << "expected: " << signal << " - got: " << error.code();
                  return;
               }
               EXPECT_TRUE( false) << "signal " << signal << " was not raised...";
            };

            auto expect_signal_raised_sleep = []( auto signal)
            {
               try
               {
                  process::sleep( std::chrono::seconds{ 2});
               }
               catch( ...)
               {
                  auto error = exception::capture();
                  EXPECT_TRUE( error.code() == signal)  << "expected: " << signal << " - got: " << error.code();
                  return;
               }
               EXPECT_TRUE( false) << "signal " << signal << " was not raised...";
            };

         } // <unnamed>
      } // local

      template< code::signal signal>
      struct holder
      {
         static constexpr code::signal get_signal() { return signal;}
      };

      template <typename H>
      struct common_signal_types : public ::testing::Test, public H
      {

      };


      using signal_type = ::testing::Types<
            holder< code::signal::interrupt>,
            holder< code::signal::quit>,
            holder< code::signal::terminate>,
            holder< code::signal::user>,
            holder< code::signal::alarm>,
            holder< code::signal::child>
       >;

      TYPED_TEST_SUITE( common_signal_types, signal_type);


      TYPED_TEST( common_signal_types, send_signal__expect_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::dispatch());

         signal::send( process::id(), TestFixture::get_signal());

         local::expect_signal_raised_sleep( TestFixture::get_signal());
      }



      TYPED_TEST( common_signal_types, block_all_signals__send_signal_X___expect_no_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::dispatch());

         {
            signal::thread::scope::Block block;

            signal::send( process::id(), TestFixture::get_signal());

            EXPECT_NO_THROW(
            {
               signal::dispatch();
            }) << "signal mask: " << signal::mask::current();
         }


         // "consume" the signal, so it's not interfering with other tests
         local::expect_signal_raised_sleep( TestFixture::get_signal());
      }


      TYPED_TEST( common_signal_types, send_signal_X__block_all_signals___expect_no_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::dispatch());

         {
            signal::send( process::id(), TestFixture::get_signal());

            signal::thread::scope::Block block;

            EXPECT_NO_THROW(
            {
               signal::dispatch();
            }) << "signal mask: " << signal::mask::current();
         }

         // "consume" the signal, so it's not interfering with other tests
         local::expect_signal_raised_sleep( TestFixture::get_signal());
      }

      TYPED_TEST( common_signal_types, block_signal_X__send_signal_X___expect_no_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::dispatch());

         {
            signal::thread::scope::Block block( { TestFixture::get_signal()});

            signal::send( process::id(), TestFixture::get_signal());

            EXPECT_NO_THROW(
            {
               signal::dispatch();
            }) << "signal mask: " << signal::mask::current();
         }

         // "consume" the signal, so it's not interfering with other tests
         local::expect_signal_raised_sleep( TestFixture::get_signal());
      }

      TYPED_TEST( common_signal_types, send_signal_X__block_signal_X___expect_no_throw)
      {
         common::unittest::Trace trace;


         EXPECT_NO_THROW( signal::dispatch());

         {
            signal::send( process::id(), TestFixture::get_signal());

            signal::thread::scope::Block block( { TestFixture::get_signal()});

            EXPECT_NO_THROW(
            {
               signal::dispatch();
            }) << "signal mask: " << signal::mask::current();
         }

         // "consume" the signal, so it's not interfering with other tests
         local::expect_signal_raised_sleep( TestFixture::get_signal());
      }

      TYPED_TEST( common_signal_types, send_signal_twice___expect_1_pending)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::dispatch());

         signal::send( process::id(), TestFixture::get_signal());
         signal::send( process::id(), TestFixture::get_signal());

         EXPECT_TRUE( signal::current::pending() == 1);
      }

      TYPED_TEST( common_signal_types, send_signal_twice__expect_1_throw)
      {
         common::unittest::Trace trace;

         signal::send( process::id(), TestFixture::get_signal());
         signal::send( process::id(), TestFixture::get_signal());

         EXPECT_THROW(
         {
            signal::dispatch();
         }, std::system_error);

         EXPECT_TRUE( signal::current::pending() == 0);

         EXPECT_NO_THROW({
            signal::dispatch();
         });
      }


      TEST( common_signal, send_terminate__expect_throw)
      {
         common::unittest::Trace trace;

         signal::send( process::id(), code::signal::terminate);

         local::expect_signal_raised_sleep( code::signal::terminate);
      }


      TEST( common_signal, send_terminate_1__child_1___expect_2_pending_signal)
      {
         common::unittest::Trace trace;

         signal::send( process::id(), code::signal::terminate);
         signal::send( process::id(), code::signal::child);

         EXPECT_TRUE( signal::current::pending() == 2);
      }

      TEST( common_signal, send_terminate_1__child_1___expect_2_throws)
      {
         common::unittest::Trace trace;

         signal::send( process::id(), code::signal::terminate);
         signal::send( process::id(), code::signal::child);

         // note that child is thrown before terminate
         local::expect_signal_raised( [](){ signal::dispatch();}, code::signal::child);
         local::expect_signal_raised( [](){ signal::dispatch();}, code::signal::terminate);

         EXPECT_TRUE( signal::current::pending() == 0);
      }


      TEST( common_signal, scope_timeout)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::dispatch());

         signal::timer::Scoped timer{ std::chrono::milliseconds{ 1}};

         local::expect_signal_raised_sleep( code::signal::alarm);
      }

      TEST( common_signal, scope_timeout__dtor_expect_no_timeouts)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::dispatch());

         EXPECT_TRUE( ! signal::timer::get());
         
         {
            signal::timer::Scoped timer{ std::chrono::milliseconds{ 1}};
            EXPECT_TRUE( signal::timer::get());
         }

         EXPECT_TRUE( ! signal::timer::get());
      }

      TEST( common_signal, scope_timeout_nested)
      {
         common::unittest::Trace trace;


         EXPECT_NO_THROW( signal::dispatch());

         signal::timer::Scoped timer1{ std::chrono::milliseconds{ 5}};

         {
            signal::timer::Scoped timer2{ std::chrono::milliseconds{ 1}};
            local::expect_signal_raised_sleep( code::signal::alarm);
         }

         local::expect_signal_raised_sleep( code::signal::alarm);
      }

   } // common
} // casual
