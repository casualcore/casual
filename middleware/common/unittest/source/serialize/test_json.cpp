//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/serialize/json.h"
#include "common/code/casual.h"

#include "../../include/test_vo.h"

namespace casual
{
   namespace common
   { 
      namespace local
      {
         namespace
         {
            template< typename T>
            void value_to_string( T&& value, std::string& string)
            {
               auto writer = serialize::json::pretty::writer();
               writer << CASUAL_NAMED_VALUE( value);
               writer.consume( string);
            }

            template< typename T>
            void string_to_strict_value( const std::string& string, T&& value)
            {
               auto reader = serialize::json::strict::reader( string);
               reader >> CASUAL_NAMED_VALUE( value);
            }

            template< typename T>
            void string_to_relaxed_value( const std::string& string, T&& value)
            {
               auto reader = serialize::json::relaxed::reader( string);
               reader >> CASUAL_NAMED_VALUE( value);
            }

         } // <unnamed>
      } // local


      TEST( common_serialize_json, relaxed_read_serializable)
      {
         common::unittest::Trace trace;
         //const std::string json = test::SimpleVO::json();

         test::SimpleVO value;

         local::string_to_relaxed_value( test::SimpleVO::json(), value);

         EXPECT_TRUE( value.m_long == 234) << "value.m_long: " << value.m_long;
         EXPECT_TRUE( value.m_string == "bla bla bla bla") << "value.m_long: " << value.m_long;
         EXPECT_TRUE( value.m_longlong == 1234567890123456789) << " value.m_longlong: " <<  value.m_longlong;

      }

      TEST( common_serialize_json, write_read_vector_long)
      {
         common::unittest::Trace trace;
         std::string json;

         {
            std::vector< long> value{ 1, 2, 3, 4, 5, 6, 7, 8};
            local::value_to_string( value, json);
         }

         {
            std::vector< long> value;
            local::string_to_relaxed_value( json, value);

            ASSERT_TRUE( value.size() == 8);
            EXPECT_TRUE( value.at( 7) == 8);
         }
      }

      TEST( common_serialize_json, array)
      {
         common::unittest::Trace trace;
         

         auto writer = serialize::json::pretty::writer();

         const std::array< char, 4> origin{ '1', '2', '3', '4' };

         {   
            writer << CASUAL_NAMED_VALUE_NAME( origin, "value");
         }

         {
            std::array< char, 4> value;
            auto json = writer.consume< platform::binary::type>();
            auto reader = serialize::json::strict::reader( json);
            reader >> CASUAL_NAMED_VALUE( value);

            EXPECT_TRUE( common::algorithm::equal( origin, value)) << CASUAL_NAMED_VALUE( value);
         }
      }

      TEST( common_serialize_json, simple_write_read)
      {
         common::unittest::Trace trace;
         std::string json;

         {
            test::SimpleVO value{ []( test::SimpleVO& v){
               v.m_long = 666;
               v.m_short = 42;
               v.m_string = "bla bla bla";
            }};
            local::value_to_string( value, json);
         }

         {
            test::SimpleVO value;
            local::string_to_relaxed_value( json, value);

            EXPECT_TRUE( value.m_long == 666);
            EXPECT_TRUE( value.m_short == 42);
            EXPECT_TRUE( value.m_string == "bla bla bla");
         }

      }

      TEST( common_serialize_json, composit_write_read)
      {
         common::unittest::Trace trace;
         std::string json;

         {
            test::Composite composite;
            composite.m_values.resize( 3);
            std::map< long, test::Composite> value { { 1, composite}, { 2, composite}};
            local::value_to_string( value, json);
         }

         {
            std::map< long, test::Composite> value;
            local::string_to_strict_value( json, value);

            ASSERT_TRUE( value.size() == 2);

            EXPECT_TRUE( value.at( 1).m_values.front().m_string == "foo");

         }
      }


      TEST( common_serialize_json, load_invalid_document__expecting_exception)
      {
         common::unittest::Trace trace;
         const std::string json
         {
            R"({
   "value":
   {
      "m_long"
   }
   }
   )"
         };

         EXPECT_CODE
         ({
            auto reader = serialize::json::strict::reader( json);
         }, code::casual::invalid_document);
      }

      TEST( common_serialize_json, read_with_invalid_long__expecting_exception)
      {
         common::unittest::Trace trace;
         const std::string json
         {
            R"(
   {
      "value":
      {
         "m_bool": true,
         "m_long": "123456",
         "m_string": "bla bla bla bla",
         "m_short": 23,
         "m_longlong": 1234567890123456789,
         "m_time": 1234567890
      }
   }
   )"
         };

         EXPECT_CODE
         ({
            test::SimpleVO value;
            local::string_to_strict_value( json, value);
         }, code::casual::invalid_node);

      }

      TEST( common_serialize_json, read_with_invalid_string__expecting_exception)
      {
         common::unittest::Trace trace;
         const std::string json
         {
            R"(
   {
      "value":
      {
         "m_bool": true,
         "m_long": 123456,
         "m_string": false,
         "m_short": 23,
         "m_longlong": 1234567890123456789,
         "m_time": 1234567890
      }
   }
   )"
         };

         EXPECT_CODE
         ({
            test::SimpleVO value;
            local::string_to_strict_value( json, value);
         }, code::casual::invalid_node);

      }

   } // common
} // casual


