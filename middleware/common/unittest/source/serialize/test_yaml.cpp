//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "../../include/test_vo.h"

#include "common/serialize/yaml.h"
#include "common/serialize/line.h"

#include "common/code/casual.h"


#include "common/algorithm.h"


namespace casual
{
   namespace common
   {
      
      namespace local
      {
         namespace
         {
            template<typename T>
            void string_to_strict_value( const std::string& string, T&& value)
            {
               auto reader = serialize::yaml::strict::reader( string);
               reader >> CASUAL_NAMED_VALUE( value);
            }

            template<typename T>
            void string_to_relaxed_value( const std::string& string, T&& value)
            {
               auto reader = serialize::yaml::relaxed::reader( string);
               reader >> CASUAL_NAMED_VALUE( value);
            }

            template<typename T>
            void value_to_string( T&& value, std::string& string)
            {
               auto writer = serialize::yaml::writer();
               writer << CASUAL_NAMED_VALUE( value);
               writer.consume( string);
            }

            struct Empty
            {
               std::vector<long> array;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( array);
               )

            };

         } // <unnamed>
      } // local


      TEST( common_serialize_yaml, read_empty_array_with_no_brackets__expecting_success)
      {

         const std::string yaml( R"(
value:
   array:
)");

         local::Empty value;

         local::string_to_strict_value( yaml, value);

         EXPECT_TRUE( value.array.empty());
      }


      TEST( common_serialize_yaml, relaxed_read_serializable)
      {
         test::SimpleVO value;

         local::string_to_relaxed_value( test::SimpleVO::yaml(), value);

         EXPECT_TRUE( value.m_long == 234) << "value.m_long: " << value.m_long;
         EXPECT_TRUE( value.m_string == "bla bla bla bla") << "value.m_long: " << value.m_long;

      }

      TEST( common_serialize_yaml, strict_read_serializable__gives_ok)
      {
         test::SimpleVO value;

         local::string_to_strict_value( test::SimpleVO::yaml(), value);

         EXPECT_TRUE( value.m_long == 234) << value.m_long;
         EXPECT_TRUE( value.m_string == "bla bla bla bla") << value.m_string;
         EXPECT_TRUE( value.m_longlong == 1234567890123456789) << value.m_longlong;
         EXPECT_TRUE( value.m_path == u8"/tmp/file.txt") << value.m_path;
      }

      TEST( common_serialize_yaml, strict_read_not_in_document__gives_throws)
      {
         auto reader = serialize::yaml::strict::reader( test::SimpleVO::yaml());

         test::SimpleVO wrongRoleName;

         EXPECT_CODE(
         {
            reader >> CASUAL_NAMED_VALUE( wrongRoleName);
         }, code::casual::invalid_node);

      }

      TEST( common_serialize_yaml, binary_array)
      {
         common::unittest::Trace trace;

         const std::array< std::byte, 4> origin{ std::byte{ '1'}, std::byte{ '2'}, std::byte{ '3'}, std::byte{ '4'}};

         auto writer = serialize::yaml::writer();
         writer << CASUAL_NAMED_VALUE_NAME( origin, "value");

         {
            auto yaml = writer.consume< platform::binary::type>();
            ASSERT_TRUE( ! yaml.empty()) << trace.compose( "size: ", yaml.size(), " - ", "data: ", yaml.data());

            std::array< std::byte, 4> value;
            auto reader = serialize::yaml::strict::reader( yaml);
            reader >> CASUAL_NAMED_VALUE( value);

            EXPECT_TRUE( common::algorithm::equal( origin, value)) << trace.compose( "value: ", value);
         }
      }

      TEST( common_serialize_yaml, write_read_large_long)
      {
         common::unittest::Trace trace;
         constexpr long value = 2170471909096019789;

         auto writer = serialize::yaml::writer();
         writer << CASUAL_NAMED_VALUE( value);

         auto yaml = writer.consume< platform::binary::type>();
         auto reader = serialize::yaml::strict::reader( yaml);
         long result{};
         reader >> CASUAL_NAMED_VALUE_NAME( result, "value");

         EXPECT_TRUE( value == result) << trace.compose( "result: ", result);
      }

      TEST( common_serialize_yaml, write_read_tuple)
      {
         common::unittest::Trace trace;
         auto origin = std::make_tuple( 1, std::string{ "foo"}, true);

         auto writer = serialize::yaml::writer();
         writer << origin;

         auto yaml = writer.consume< platform::binary::type>();
         auto reader = serialize::yaml::strict::reader( yaml);
         auto result = decltype( origin){};
         reader >> result;

         EXPECT_TRUE( origin == result) << trace.compose( "result: ", result);
      }

      TEST( common_serialize_yaml, write_read_vector_pod)
      {
         common::unittest::Trace trace;
         std::string yaml;

         {
            std::vector< long> values = { 1, 2, 34, 45, 34, 34, 23};
            local::value_to_string( values, yaml);
         }

         std::vector< long> values;
         local::string_to_relaxed_value( yaml, values);

         ASSERT_TRUE( values.size() == 7) << values.size();
         EXPECT_TRUE( values.at( 0) == 1);
         EXPECT_TRUE( values.at( 1) == 2) << trace.compose( "values: ", values);
         EXPECT_TRUE( values.at( 2) == 34) << values.at( 2);
         EXPECT_TRUE( values.at( 3) == 45);
         EXPECT_TRUE( values.at( 4) == 34);
         EXPECT_TRUE( values.at( 5) == 34);
         EXPECT_TRUE( values.at( 6) == 23);
      }

      TEST( common_serialize_yaml, write_read_vector_serializable)
      {
         common::unittest::Trace trace;
         std::string yaml;

         {
            std::vector< test::SimpleVO> values{
               { []( test::SimpleVO& v){
                  v.m_long = 2342342;
                  v.m_string = "one two three";
                  v.m_short = 123; }
               },
               { []( test::SimpleVO& v){
                  v.m_long = 234234;
                  v.m_string = "four five six";
                  v.m_short = 456; }
               }
            };
            local::value_to_string( values, yaml);
         }

         std::vector< test::SimpleVO> values;
         local::string_to_relaxed_value( yaml, values);

         ASSERT_TRUE( values.size() == 2);
         EXPECT_TRUE( values.at( 0).m_short == 123);
         EXPECT_TRUE( values.at( 0).m_string == "one two three");
         EXPECT_TRUE( values.at( 1).m_short == 456);
         EXPECT_TRUE( values.at( 1).m_string == "four five six");

      }



      TEST( common_serialize_yaml, write_read_map_complex)
      {
         common::unittest::Trace trace;
         std::string yaml;

         {
            std::map< long, test::Composite> values;
            values[ 10].m_string = "kalle";
            values[ 10].m_values = {
                  { []( test::SimpleVO& v){ v.m_long = 11111; v.m_string = "one"; v.m_short = 1;}},
                  { []( test::SimpleVO& v){ v.m_long = 22222; v.m_string = "two"; v.m_short = 2;}}
            };
            values[ 2342].m_string = "Charlie";
            values[ 2342].m_values = {
                  { []( test::SimpleVO& v){ v.m_long = 33333; v.m_string = "three"; v.m_short = 3;}},
                  { []( test::SimpleVO& v){ v.m_long = 444444; v.m_string = "four"; v.m_short = 4;}}
            };

            local::value_to_string( values, yaml);
         }

         std::map< long, test::Composite> values;
         local::string_to_relaxed_value( yaml, values);


         ASSERT_TRUE( values.size() == 2) << trace.compose( "values: ", values);
         EXPECT_TRUE( values.at( 10).m_string == "kalle");
         EXPECT_TRUE( values.at( 10).m_values.at( 0).m_short == 1);
         EXPECT_TRUE( values.at( 10).m_values.at( 0).m_string == "one");
         EXPECT_TRUE( values.at( 10).m_values.at( 1).m_short == 2);
         EXPECT_TRUE( values.at( 10).m_values.at( 1).m_string == "two");

         EXPECT_TRUE( values.at( 2342).m_string == "Charlie");
         EXPECT_TRUE( values.at( 2342).m_values.at( 0).m_short == 3);
         EXPECT_TRUE( values.at( 2342).m_values.at( 0).m_string == "three");
         EXPECT_TRUE( values.at( 2342).m_values.at( 1).m_short == 4);
         EXPECT_TRUE( values.at( 2342).m_values.at( 1).m_string == "four");

      }

      TEST( common_serialize_yaml, write_read_binary)
      {
         common::unittest::Trace trace;
         std::string yaml;

         const auto origin = unittest::random::binary( 24);

         {
            test::Binary value;

            value.m_long = 23;
            value.m_string = "Charlie";
            value.m_binary = origin;

            local::value_to_string( value, yaml);
         }

         test::Binary value;
         local::string_to_relaxed_value( yaml, value);

         EXPECT_TRUE( algorithm::equal( value.m_binary, origin));
      }


      TEST( common_serialize_yaml, read_invalid_document__expecting_exception)
      {
         common::unittest::Trace trace;

         const std::string yaml{ "   " };

         EXPECT_CODE
         ({
            serialize::yaml::strict::reader( yaml);
         }, code::casual::invalid_document);
      }

      TEST( common_serialize_yaml, read_invalid_bool__expecting_exception)
      {
         constexpr auto yaml = R"(
value:
   m_bool: jajjemensan
   m_long: 123
   m_string: bla bla bla bla
   m_short: 23
   m_longlong: 1234567890123456789
   m_time: 1234567890
)";

         EXPECT_CODE
         ({
            test::SimpleVO value;
            local::string_to_strict_value( yaml, value);
         }, code::casual::invalid_document);
      }



   } // common
} // casual


