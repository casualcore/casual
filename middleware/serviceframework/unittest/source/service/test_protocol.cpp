//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>

#include "serviceframework/service/protocol.h"
#include "common/serialize/create.h"

#include "../../include/test_vo.h"

#include "common/unittest.h"

namespace casual
{
   using namespace common;
   
   namespace serviceframework
   {
      namespace local
      {
         namespace 
         {
            auto prepare( std::string protocol)
            {
               serviceframework::service::protocol::parameter_type result;
               result.payload.type = std::move( protocol);

               return result;
            }

            template< typename T>
            auto prepare( std::string protocol, T& value)
            {
               auto result = prepare( std::move( protocol));

               auto writer = common::serialize::create::writer::from( result.payload.type);
               writer << CASUAL_NAMED_VALUE( value);
               writer.consume( result.payload.memory);

               return result;
            }
         } // <unnamed>
      }

      class protocol : public ::testing::TestWithParam< std::string>
      {
      };

      TEST_P( protocol, deduce)
      {
         common::unittest::Trace trace;

         auto protocol = service::protocol::deduce( local::prepare( GetParam()));
         EXPECT_TRUE( protocol.type() == GetParam());
      }

      TEST_P( protocol, simple_input_deserialize)
      {
         common::unittest::Trace trace;

         long some_long = 42;

         auto parameter = local::prepare( GetParam(), some_long);

         EXPECT_TRUE( parameter.payload.memory.size() > 0) << "size: " << parameter.payload.memory.size();

         auto protocol = service::protocol::deduce( std::move( parameter));

         {
            long value = 0;
            protocol >> CASUAL_NAMED_VALUE( value);

            EXPECT_TRUE( value == some_long) << "value: " << value;
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

         auto protocol = service::protocol::deduce( local::prepare( GetParam(), vo));

         {
            test::SimpleVO value;
            protocol >> CASUAL_NAMED_VALUE( value);

            EXPECT_TRUE( value.m_bool == false);
            EXPECT_TRUE( value.m_long == 42);
            EXPECT_TRUE( value.m_string == "poop");
         }
      }

      TEST_P( protocol, output_serialize)
      {
         common::unittest::Trace trace;

         auto protocol = service::protocol::deduce( local::prepare( GetParam()));

         {
            test::SimpleVO value;
            value.m_bool = false;
            value.m_long = 42;
            value.m_string = "poop";

            protocol << CASUAL_NAMED_VALUE( value);
         }

         auto result = protocol.finalize();

         {
            test::SimpleVO value;

            auto reader = common::serialize::create::reader::strict::from( result.payload.type, result.payload.memory);
            reader >> CASUAL_NAMED_VALUE( value);

            EXPECT_TRUE( value.m_bool == false);
            EXPECT_TRUE( value.m_long == 42);
            EXPECT_TRUE( value.m_string == "poop");
         }
      }

      namespace local
      {
         namespace
         {
            struct Value
            {
               long m_long{};
               std::string m_string;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( m_long);
                  CASUAL_SERIALIZE( m_string);
               )
            };
         } // <unnamed>
      } // local

      TEST_P( protocol, input_output_parameter_log)
      {
         common::unittest::Trace trace;

         // make sure parameter is active
         common::log::stream::activate( "parameter");
         ASSERT_TRUE( common::log::category::parameter);

         // capture parameter
         std::ostringstream parameter;
         auto reset = common::unittest::capture::output::stream( common::log::category::parameter, parameter);


         local::Value value{ 42, "foo"};
         auto protocol = service::protocol::deduce( local::prepare( GetParam(), value));

         // input
         {
            local::Value value{};
            protocol >> CASUAL_NAMED_VALUE( value);
            EXPECT_TRUE( value.m_long == 42) << "value.m_long: " << value.m_long;
            EXPECT_TRUE( value.m_string == "foo") << "value.m_string: " << value.m_string;
         }

         // output
         {
            local::Value result{ 43, "bar"};
            protocol << CASUAL_NAMED_VALUE( result);
         }
            
         protocol.finalize();

         auto log = std::move( parameter).str();

         // remove all ws and " to make it less likely to fail if we change format.
         algorithm::trim( log, algorithm::remove( algorithm::remove( log, '"'), ' '));

         constexpr auto expected = R"(value:{m_long:42,m_string:foo},result:{m_long:43,m_string:bar}
)";

         EXPECT_TRUE( log == expected) << "log: " << log;
         
      }

      INSTANTIATE_TEST_CASE_P( casual_sf_service,
            protocol,
            ::testing::Values(
               common::buffer::type::xml(),
               common::buffer::type::json(),
               common::buffer::type::yaml(),
               common::buffer::type::ini()
               //common::buffer::type::binary()
            )
      );
   } // serviceframework
} // casual



