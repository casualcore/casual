//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>


#include "common/code/xatmi.h"
#include "common/exception/xatmi.h"
#include "common/exception/handle.h"




namespace casual
{
   namespace common
   {
         namespace local
      {
         namespace
         {
            struct UnknownException
            {

            };
         } // <unnamed>
      } // local

      TEST( casual_common_error, catch_unknown)
      {
         EXPECT_NO_THROW({
            try
            {
               throw local::UnknownException{};
            }
            catch( ...)
            {
               exception::handle();
            }
         });
      }
   } // common



   template< typename E, common::code::xatmi code>
   struct holder
   {
      typedef E exception_type;
      static common::code::xatmi getCode() { return code;}
   };

   template <typename H>
   struct casual_common_error_xatmi : public ::testing::Test, public H
   {

   };




   typedef ::testing::Types<
         holder< common::exception::xatmi::no::Message, common::code::xatmi::no_message>,
         holder< common::exception::xatmi::Limit, common::code::xatmi::limit>,
         holder< common::exception::xatmi::invalid::Argument, common::code::xatmi::argument>,
         holder< common::exception::xatmi::os::Error, common::code::xatmi::os>,
         holder< common::exception::xatmi::Protocoll, common::code::xatmi::protocol>,
         holder< common::exception::xatmi::invalid::Descriptor, common::code::xatmi::descriptor>,
         holder< common::exception::xatmi::service::Error, common::code::xatmi::service_error>,
         holder< common::exception::xatmi::service::Fail, common::code::xatmi::service_fail>,
         holder< common::exception::xatmi::service::no::Entry, common::code::xatmi::no_entry>,
         holder< common::exception::xatmi::service::Advertised, common::code::xatmi::service_advertised>,
         holder< common::exception::xatmi::System, common::code::xatmi::system>,
         holder< common::exception::xatmi::Timeout, common::code::xatmi::timeout>,
         holder< common::exception::xatmi::transaction::Support, common::code::xatmi::transaction>,
         holder< common::exception::xatmi::Signal, common::code::xatmi::signal>,
         holder< common::exception::xatmi::buffer::type::Input, common::code::xatmi::buffer_input>,
         holder< common::exception::xatmi::buffer::type::Output, common::code::xatmi::buffer_output>
    > xatmi_exceptions;

   TYPED_TEST_CASE(casual_common_error_xatmi, xatmi_exceptions);

   TYPED_TEST( casual_common_error_xatmi, xatmi_throw__expect_error_number)
	{
      typedef typename TestFixture::exception_type exception_type;

      try
      {
         throw exception_type( "some string");
      }
      catch( ...)
      {
         EXPECT_TRUE( common::exception::xatmi::handle() == TestFixture::getCode());
      }
	}

   TYPED_TEST( casual_common_error_xatmi, xatmi__expect_code)
   {
      typedef typename TestFixture::exception_type exception_type;

      try
      {
         throw exception_type( "some string");
      }
      catch( const common::exception::xatmi::exception& exception)
      {
         EXPECT_TRUE( exception.type() == TestFixture::getCode());
      }
   }

   template <typename H>
   struct casual_common_error_tx : public ::testing::Test, public H
   {

   };
  

   /*
   typedef ::testing::Types<
         holder< common::exception::tx::OutsideTransaction, TX_OUTSIDE, Severity::user>,
         holder< common::exception::tx::RolledBack, TX_ROLLBACK, Severity::information>,
         holder< common::exception::tx::HeuristicallyCommitted, TX_COMMITTED, Severity::information>,
         holder< common::exception::tx::Mixed, TX_MIXED, Severity::information>,
         holder< common::exception::tx::Hazard, TX_HAZARD, Severity::error>,
         holder< common::exception::tx::ProtocollError, TX_PROTOCOL_ERROR, Severity::user>,
         holder< common::exception::tx::Error, TX_ERROR, Severity::error>,
         holder< common::exception::tx::Fail, TX_FAIL, Severity::error>,
         holder< common::exception::tx::InvalidArguments, TX_EINVAL, Severity::user>,
         holder< common::exception::tx::no_begin::RolledBack, TX_ROLLBACK_NO_BEGIN, Severity::user>,
         holder< common::exception::tx::no_begin::Mixed, TX_MIXED_NO_BEGIN, Severity::information>,
         holder< common::exception::tx::no_begin::Haxard, TX_HAZARD_NO_BEGIN, Severity::error>,
         holder< common::exception::tx::no_begin::Committed, TX_COMMITTED_NO_BEGIN, Severity::user>
    > tx_exceptions;

   TYPED_TEST_CASE(casual_common_error_tx, tx_exceptions);

   TYPED_TEST( casual_common_error_tx, tx_expect_error_number)
   {
      typedef typename TestFixture::exception_type exception_type;

      // TODO: make a handler
      exception_type exception( "some string");

      EXPECT_TRUE( exception.code() == TestFixture::getCode());
   }

   TYPED_TEST( casual_common_error_tx, tx__expect_severity)
   {
      typedef typename TestFixture::exception_type exception_type;

      exception_type exception( "some string");

      EXPECT_TRUE( exception.severity() == TestFixture::getSeverity());

   }
   */

   namespace common
   {
      TEST( casual_common_error_xatmi, error_code)
      {
         std::error_code code = code::xatmi::no_entry;

         EXPECT_TRUE( code) << "code: " << code.message();
         EXPECT_TRUE( code.value() == static_cast< int>( code::xatmi::no_entry)) << "code: " << code;

      }

      TEST( casual_common_error_xatmi, throw_no_entry)
      {
         try
         {
            throw exception::xatmi::service::no::Entry{};
         }
         catch( const exception::xatmi::service::no::Entry& e)
         {
            EXPECT_TRUE( e.code()) << "code: " << e.code().message();
            EXPECT_TRUE( e.code().value() == static_cast< int>( code::xatmi::no_entry));
         }
      }
   } // common


} // casual
