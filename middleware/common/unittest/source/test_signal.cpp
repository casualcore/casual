//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/signal.h"
#include "common/process.h"
#include "common/exception/signal.h"
#include "common/exception/system.h"


namespace casual
{
   namespace common
   {
      template< typename E, code::signal signal>
      struct holder
      {
         using exception_type = E;
         static constexpr code::signal get_signal() { return signal;}
      };

      template <typename H>
      struct casual_common_signal_types : public ::testing::Test, public H
      {

      };


      using signal_type = ::testing::Types<

            holder< common::exception::signal::Terminate, code::signal::interrupt>,
            holder< common::exception::signal::Terminate, code::signal::quit>,
            holder< common::exception::signal::Terminate, code::signal::terminate>,

            holder< common::exception::signal::User, code::signal::user>,

            holder< common::exception::signal::Timeout, code::signal::alarm>,

            holder< common::exception::signal::child::Terminate, code::signal::child>
       >;

      TYPED_TEST_CASE( casual_common_signal_types, signal_type);


      TYPED_TEST( casual_common_signal_types, send_signal__expect_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::dispatch());

         signal::send( process::id(), TestFixture::get_signal());

         using exception_type = typename TestFixture::exception_type;

         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});
         }, exception_type);
      }



      TYPED_TEST( casual_common_signal_types, block_all_signals__send_signal_X___expect_no_throw)
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

         using exception_type = typename TestFixture::exception_type;

         // "consume" the signal, so it's not interfering with other tests
         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});
         }, exception_type);

      }


      TYPED_TEST( casual_common_signal_types, send_signal_X__block_all_signals___expect_no_throw)
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

         using exception_type = typename TestFixture::exception_type;

         // "consume" the signal, so it's not interfering with other tests
         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});
         }, exception_type);
      }

      TYPED_TEST( casual_common_signal_types, block_signal_X__send_signal_X___expect_no_throw)
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

         using exception_type = typename TestFixture::exception_type;

         // "consume" the signal, so it's not interfering with other tests
         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});
         }, exception_type);
      }

      TYPED_TEST( casual_common_signal_types, send_signal_X__block_signal_X___expect_no_throw)
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

         using exception_type = typename TestFixture::exception_type;

         // "consume" the signal, so it's not interfering with other tests
         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});
         }, exception_type);
      }

      TYPED_TEST( casual_common_signal_types, send_signal_twice___expect_1_pending)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::dispatch());

         signal::send( process::id(), TestFixture::get_signal());
         signal::send( process::id(), TestFixture::get_signal());

         EXPECT_TRUE( signal::current::pending() == 1);
      }

      TYPED_TEST( casual_common_signal_types, send_signal_twice__expect_1_throw)
      {
         common::unittest::Trace trace;

         signal::send( process::id(), TestFixture::get_signal());
         signal::send( process::id(), TestFixture::get_signal());

         using exception_type = typename TestFixture::exception_type;

         EXPECT_THROW(
         {
            signal::dispatch();
         }, exception_type);

         EXPECT_TRUE( signal::current::pending() == 0);

         EXPECT_NO_THROW({
            signal::dispatch();
         });
      }


      TEST( casual_common_signal, send_terminate__expect_throw)
      {
         common::unittest::Trace trace;


         signal::send( process::id(), code::signal::terminate);

         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});

         }, exception::signal::Terminate) << "signal mask: " << signal::mask::current();
      }



      TEST( casual_common_signal, send_terminate_1__child_1___expect_2_pending_signal)
      {
         common::unittest::Trace trace;

         signal::send( process::id(), code::signal::terminate);
         signal::send( process::id(), code::signal::child);

         EXPECT_TRUE( signal::current::pending() == 2);
      }

      TEST( casual_common_signal, send_terminate_1__child_1___expect_2_throws)
      {
         common::unittest::Trace trace;

         signal::send( process::id(), code::signal::terminate);
         signal::send( process::id(), code::signal::child);

         // note that child is thrown before terminate
         EXPECT_THROW(
         {
            signal::dispatch();
         }, exception::signal::child::Terminate);

         EXPECT_THROW(
         {
            signal::dispatch();
         }, exception::signal::Terminate);

         EXPECT_TRUE( signal::current::pending() == 0);
      }


      TEST( casual_common_signal, scope_timeout)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::dispatch());

         signal::timer::Scoped timer{ std::chrono::milliseconds{ 1}};

         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});
         }, exception::signal::Timeout);
      }

      TEST( casual_common_signal, nested_timeout)
      {
         common::unittest::Trace trace;


         EXPECT_NO_THROW( signal::dispatch());

         signal::timer::Scoped timer1{ std::chrono::milliseconds{ 5}};

         {
            signal::timer::Scoped timer2{ std::chrono::milliseconds{ 1}};

            EXPECT_THROW(
            {
               process::sleep( std::chrono::seconds{ 2});
            }, exception::signal::Timeout);
         }

         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});
         }, exception::signal::Timeout);
      }




   } // common

} // casual
