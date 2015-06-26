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
            common::error::handler();
         }
      });
   }


   template< typename E, int code, common::log::category::Type category>
   struct holder
   {
      typedef E exception_type;
      static int getCode() { return code;}
      static int getGategory() { return static_cast< int>( category);}
      //static const char* getString() { return string;}

   };

   template <typename H>
   struct casual_common_error_xatmi : public ::testing::Test, public H
   {

   };




   typedef ::testing::Types<
         holder< common::exception::xatmi::NoMessage, TPEBLOCK, common::log::category::Type::debug>,
         holder< common::exception::xatmi::LimitReached, TPELIMIT, common::log::category::Type::information>,
         holder< common::exception::xatmi::InvalidArguments, TPEINVAL, common::log::category::Type::debug>,
         holder< common::exception::xatmi::OperatingSystemError, TPEOS, common::log::category::Type::error>,
         holder< common::exception::xatmi::ProtocollError, TPEPROTO, common::log::category::Type::error>,
         holder< common::exception::xatmi::service::InvalidDescriptor, TPEBADDESC, common::log::category::Type::debug>,
         holder< common::exception::xatmi::service::Error, TPESVCERR, common::log::category::Type::error>,
         holder< common::exception::xatmi::service::Fail, TPESVCFAIL, common::log::category::Type::debug>,
         holder< common::exception::xatmi::service::NoEntry, TPENOENT, common::log::category::Type::debug>,
         holder< common::exception::xatmi::service::AllreadyAdvertised, TPEMATCH, common::log::category::Type::debug>,
         holder< common::exception::xatmi::SystemError, TPESYSTEM, common::log::category::Type::error>,
         holder< common::exception::xatmi::Timeout, TPETIME, common::log::category::Type::debug>,
         holder< common::exception::xatmi::TransactionNotSupported, TPETRAN, common::log::category::Type::debug>,
         holder< common::exception::xatmi::Signal, TPGOTSIG, common::log::category::Type::information>,
         holder< common::exception::xatmi::buffer::TypeNotSupported, TPEITYPE, common::log::category::Type::debug>,
         holder< common::exception::xatmi::buffer::TypeNotExpected, TPEOTYPE, common::log::category::Type::debug>
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

   TYPED_TEST( casual_common_error_xatmi, xatmi__expect_category)
   {
      typedef typename TestFixture::exception_type exception_type;

      //exception_type exception( "some string");

      EXPECT_TRUE( exception_type::category_value == TestFixture::getGategory());

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

} // casual
