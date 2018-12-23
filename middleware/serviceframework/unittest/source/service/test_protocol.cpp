//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>


#include "serviceframework/service/protocol.h"
#include "serviceframework/archive/create.h"


#include "../../include/test_vo.h"


#include "common/unittest.h"

namespace casual
{
   namespace serviceframework
   {
      
      namespace local
      {
         namespace 
         {
            //serviceframework::service::protocol::parameter_type prepare( std::string protocol)
            auto prepare( std::string protocol)
            {
               serviceframework::service::protocol::parameter_type result;
               result.payload.type = std::move( protocol);

               return result;
            }

            template< typename T>
            serviceframework::service::protocol::parameter_type prepare( std::string protocol, T& value)
            //auto prepare( std::string protocol, T& value) // why does not auto return type deduction work?!
            {
               auto result = prepare( std::move( protocol));

               auto writer = archive::create::writer::from( result.payload.type, result.payload.memory);
               writer << CASUAL_MAKE_NVP( value);

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
            protocol >> CASUAL_MAKE_NVP( value);

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
            protocol >> CASUAL_MAKE_NVP( value);

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

            protocol << CASUAL_MAKE_NVP( value);
         }

         auto result = protocol.finalize();

         {
            test::SimpleVO value;

            auto reader = archive::create::reader::strict::from( result.payload.type, result.payload.memory);
            reader >> CASUAL_MAKE_NVP( value);

            EXPECT_TRUE( value.m_bool == false);
            EXPECT_TRUE( value.m_long == 42);
            EXPECT_TRUE( value.m_string == "poop");
         }
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


