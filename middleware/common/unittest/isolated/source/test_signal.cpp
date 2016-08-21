//!
//! caual
//!
#include "common/unittest.h"

#include "common/signal.h"
#include "common/process.h"
#include "common/exception.h"
#include "common/trace.h"

namespace casual
{
   namespace common
   {
      template< typename E, signal::Type signal>
      struct holder
      {
         typedef E exception_type;
         static signal::Type get_signal() { return signal;}
         //static const char* getString() { return string;}

      };

      template <typename H>
      struct casual_common_signal_types : public ::testing::Test, public H
      {

      };




      typedef ::testing::Types<

            holder< common::exception::signal::Terminate, signal::Type::interupt>,
            holder< common::exception::signal::Terminate, signal::Type::quit>,
            holder< common::exception::signal::Terminate, signal::Type::terminate>,

            holder< common::exception::signal::User, signal::Type::user>,

            holder< common::exception::signal::Timeout, signal::Type::alarm>,

            holder< common::exception::signal::child::Terminate, signal::Type::child>,

            holder< common::exception::signal::Pipe, signal::Type::pipe>
       > signal_type;

      TYPED_TEST_CASE( casual_common_signal_types, signal_type);


      TYPED_TEST( casual_common_signal_types, send_signal__expect_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( signal::handle());

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

         EXPECT_NO_THROW( signal::handle());

         {
            signal::thread::scope::Block block;

            signal::send( process::id(), TestFixture::get_signal());

            EXPECT_NO_THROW(
            {
               signal::handle();
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

         EXPECT_NO_THROW( signal::handle());

         {

            signal::send( process::id(), TestFixture::get_signal());

            signal::thread::scope::Block block;

            EXPECT_NO_THROW(
            {
               signal::handle();
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

         EXPECT_NO_THROW( signal::handle());

         {
            signal::thread::scope::Block block( { TestFixture::get_signal()});

            signal::send( process::id(), TestFixture::get_signal());

            EXPECT_NO_THROW(
            {
               signal::handle();
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


         EXPECT_NO_THROW( signal::handle());

         {
            signal::send( process::id(), TestFixture::get_signal());

            signal::thread::scope::Block block( { TestFixture::get_signal()});

            EXPECT_NO_THROW(
            {
               signal::handle();
            }) << "signal mask: " << signal::mask::current();
         }

         using exception_type = typename TestFixture::exception_type;

         // "consume" the signal, so it's not interfering with other tests
         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});
         }, exception_type);
      }


      TEST( casual_common_signal, send_terminate__expect_throw)
      {
         common::unittest::Trace trace;


         signal::send( process::id(), signal::Type::terminate);

         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});

         }, exception::signal::Terminate) << "signal mask: " << signal::mask::current();
      }


      TEST( casual_common_signal, scope_timeout)
      {
         common::unittest::Trace trace;


         EXPECT_NO_THROW( signal::handle());

         signal::timer::Scoped timer{ std::chrono::milliseconds{ 1}};

         EXPECT_THROW(
         {
            process::sleep( std::chrono::seconds{ 2});

         }, exception::signal::Timeout);
      }

      TEST( casual_common_signal, nested_timeout)
      {
         common::unittest::Trace trace;


         EXPECT_NO_THROW( signal::handle());

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
