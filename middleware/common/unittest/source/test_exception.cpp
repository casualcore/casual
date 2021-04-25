//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"


#include "common/code/raise.h"
#include "common/code/xatmi.h"
#include "common/code/casual.h"




namespace casual
{
   namespace common
   {

      template< typename Code, Code value>
      struct holder
      {
         constexpr static Code code() { return value;}
      };

      template< common::code::xatmi code>
      using xatmi_holder = holder< common::code::xatmi, code>;

      template< common::code::casual code>
      using casual_holder = holder< common::code::casual, code>;

      template <typename H>
      struct casual_common_error : public ::testing::Test, public H
      {

      };


      using exceptions = ::testing::Types<
            xatmi_holder< common::code::xatmi::no_message>,
            xatmi_holder< common::code::xatmi::limit>,
            xatmi_holder< common::code::xatmi::argument>,
            xatmi_holder< common::code::xatmi::os>,
            xatmi_holder< common::code::xatmi::protocol>,
            xatmi_holder< common::code::xatmi::descriptor>,
            xatmi_holder< common::code::xatmi::service_error>,
            xatmi_holder< common::code::xatmi::service_fail>,
            xatmi_holder< common::code::xatmi::no_entry>,
            xatmi_holder< common::code::xatmi::service_advertised>,
            xatmi_holder< common::code::xatmi::system>,
            xatmi_holder< common::code::xatmi::timeout>,
            xatmi_holder< common::code::xatmi::transaction>,
            xatmi_holder< common::code::xatmi::signal>,
            xatmi_holder< common::code::xatmi::buffer_input>,
            xatmi_holder< common::code::xatmi::buffer_output>,

            casual_holder< common::code::casual::failed_transcoding>,
            casual_holder< common::code::casual::invalid_argument>,
            casual_holder< common::code::casual::invalid_configuration>,
            casual_holder< common::code::casual::invalid_document>,
            casual_holder< common::code::casual::invalid_node>,
            casual_holder< common::code::casual::invalid_path>,
            casual_holder< common::code::casual::invalid_semantics>,
            casual_holder< common::code::casual::invalid_version>,
            casual_holder< common::code::casual::shutdown>,
            casual_holder< common::code::casual::communication_protocol>,
            casual_holder< common::code::casual::communication_refused>,
            casual_holder< common::code::casual::communication_retry>,
            casual_holder< common::code::casual::communication_unavailable>
      >;

      TYPED_TEST_SUITE( casual_common_error, exceptions);

      TYPED_TEST( casual_common_error, throw__expect_error_number)
      {
         unittest::Trace trace;

         EXPECT_CODE( 
         {
            code::raise::error( TestFixture::code());
         }, TestFixture::code());
         
      }
/*
      TYPED_TEST( casual_common_error, xatmi__expect_code)
      {
         unittest::Trace trace;

         using exception_type = typename TestFixture::exception_type;

         try
         {
            throw exception_type( "some string");
         }
         catch( const common::exception::xatmi::exception& exception)
         {
            EXPECT_TRUE( exception.type() == TestFixture::getCode());
         }
      }
      */
  
   } // common


} // casual
