//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>


#include "serviceframework/archive/maker.h"
#include "serviceframework/service/protocol.h"


#include "../../include/test_vo.h"


#include "common/unittest.h"

namespace casual
{



   namespace local
   {
      namespace
      {

         serviceframework::service::protocol::parameter_type prepare( std::string protocol)
         {
            serviceframework::service::protocol::parameter_type result;
            result.payload.type = std::move( protocol);

            return result;
         }

         template< typename T>
         serviceframework::service::protocol::parameter_type prepare( std::string protocol, T& value)
         {
            auto result = prepare( std::move( protocol));

            auto writer = serviceframework::archive::writer::from::buffer( result.payload.memory, result.payload.type);
            writer << CASUAL_MAKE_NVP( value);

            return result;
         }
      }
   }

   class protocol : public ::testing::TestWithParam< std::string>
   {
   };

   TEST_P( protocol, deduce)
   {
      common::unittest::Trace trace;

      auto protocol = serviceframework::service::protocol::deduce( local::prepare( GetParam()));
      EXPECT_TRUE( protocol.type() == GetParam());
   }

   TEST_P( protocol, simple_input_deserialize)
   {
      common::unittest::Trace trace;

      long some_long = 42;

      auto protocol = serviceframework::service::protocol::deduce( local::prepare( GetParam(), some_long));

      {
         long value = 0;
         protocol >> CASUAL_MAKE_NVP( value);

         EXPECT_TRUE( value == some_long);
      }
   }

   TEST_P( protocol, input_deserialize)
   {
      common::unittest::Trace trace;

      test::SimpleVO vo;
      {
         vo.m_bool = false;
         vo.m_long = 42;
         vo.m_string = "poop";
      }

      auto protocol = serviceframework::service::protocol::deduce( local::prepare( GetParam(), vo));

      {
         test::SimpleVO value;
         protocol >> CASUAL_MAKE_NVP( value);

         EXPECT_TRUE( value.m_bool == false);
         EXPECT_TRUE( value.m_long == 42);
         EXPECT_TRUE( value.m_string == "poop");
      }
   }

   TEST_P( protocol, output_serialize)
   {
      common::unittest::Trace trace;

      auto protocol = serviceframework::service::protocol::deduce( local::prepare( GetParam()));

      {
         test::SimpleVO value;
         value.m_bool = false;
         value.m_long = 42;
         value.m_string = "poop";

         protocol << CASUAL_MAKE_NVP( value);
      }

      auto result = protocol.finalize();

      {
         test::SimpleVO value;

         auto reader = serviceframework::archive::reader::from::buffer( result.payload.memory, result.payload.type);
         reader >> CASUAL_MAKE_NVP( value);

         EXPECT_TRUE( value.m_bool == false);
         EXPECT_TRUE( value.m_long == 42);
         EXPECT_TRUE( value.m_string == "poop");
      }
   }

   INSTANTIATE_TEST_CASE_P( caual_sf_service,
         protocol,
         ::testing::Values(
            common::buffer::type::xml(),
            common::buffer::type::json(),
            common::buffer::type::yaml(),
            common::buffer::type::ini()
            //common::buffer::type::binary()
         )
   );

}



