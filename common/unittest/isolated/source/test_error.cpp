//!
//! test_error.cpp
//!
//! Created on: Aug 3, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/exception.h"





namespace casual
{

   enum class Severity : int
   {
      error = common::platform::cLOG_error,
      information = common::platform::cLOG_info,
      user = common::platform::cLOG_debug
   };

   template< typename E, int code, Severity severity>
   struct holder
   {
      typedef E exception_type;
      static int getCode() { return code;}
      static int getSeverity() { return static_cast< int>( severity);}
      //static const char* getString() { return string;}

   };

   template <typename H>
   struct casual_common_error_xatmi : public ::testing::Test, public H
   {

   };




   typedef ::testing::Types<
         holder< common::exception::xatmi::NoMessage, TPEBLOCK, Severity::user>,
         holder< common::exception::xatmi::LimitReached, TPELIMIT, Severity::information>,
         holder< common::exception::xatmi::InvalidArguments, TPEINVAL, Severity::user>,
         holder< common::exception::xatmi::OperatingSystemError, TPEOS, Severity::error>,
         holder< common::exception::xatmi::ProtocollError, TPEPROTO, Severity::error>,
         holder< common::exception::xatmi::service::InvalidDescriptor, TPEBADDESC, Severity::user>,
         holder< common::exception::xatmi::service::Error, TPESVCERR, Severity::error>,
         holder< common::exception::xatmi::service::Fail, TPESVCFAIL, Severity::user>,
         holder< common::exception::xatmi::service::NoEntry, TPENOENT, Severity::user>,
         holder< common::exception::xatmi::service::AllreadyAdvertised, TPEMATCH, Severity::user>,
         holder< common::exception::xatmi::SystemError, TPESYSTEM, Severity::error>,
         holder< common::exception::xatmi::Timeout, TPETIME, Severity::user>,
         holder< common::exception::xatmi::TransactionNotSupported, TPETRAN, Severity::user>,
         holder< common::exception::xatmi::Signal, TPGOTSIG, Severity::information>,
         holder< common::exception::xatmi::buffer::TypeNotSupported, TPEITYPE, Severity::user>,
         holder< common::exception::xatmi::buffer::TypeNotExpected, TPEOTYPE, Severity::user>
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
         EXPECT_TRUE( common::error::handler() == TestFixture::getCode());
      }
	}

   TYPED_TEST( casual_common_error_xatmi, xatmi__expect_severity)
   {
      typedef typename TestFixture::exception_type exception_type;

      exception_type exception( "some string");

      EXPECT_TRUE( exception.severity() == TestFixture::getSeverity());

   }

   template <typename H>
   struct casual_common_error_tx : public ::testing::Test, public H
   {

   };

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

} // casual
