//!
//! casual
//!

#include <gtest/gtest.h>


#include "sf/archive/json.h"

#include "../../include/test_vo.h"

namespace casual
{

   namespace local
   {
      namespace
      {

         template< typename T>
         void value_to_string( T&& value, std::string& string)
         {
            sf::archive::json::Save save;
            sf::archive::json::Writer writer( save());

            writer << CASUAL_MAKE_NVP( value);

            save( string);
         }

         template< typename T>
         void string_to_strict_value( const std::string& string, T&& value)
         {
            sf::archive::json::Load load;

            sf::archive::json::Reader reader( load( string));

            reader >> CASUAL_MAKE_NVP( value);
         }

         template< typename T>
         void string_to_relaxed_value( const std::string& string, T&& value)
         {
            sf::archive::json::Load load;

            sf::archive::json::relaxed::Reader reader( load( string));

            reader >> CASUAL_MAKE_NVP( value);
         }

      } // <unnamed>


   } // local


   TEST( casual_sf_json_archive, relaxed_read_serializible)
   {
      //const std::string json = test::SimpleVO::json();

      test::SimpleVO value;

      local::string_to_relaxed_value( test::SimpleVO::json(), value);

      EXPECT_TRUE( value.m_long == 234) << "value.m_long: " << value.m_long;
      EXPECT_TRUE( value.m_string == "bla bla bla bla") << "value.m_long: " << value.m_long;
      EXPECT_TRUE( value.m_longlong == 1234567890123456789) << " value.m_longlong: " <<  value.m_longlong;

   }

   TEST( casual_sf_json_archive, write_read_vector_long)
   {
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

   TEST( casual_sf_json_archive, DISABLED_simple_write_read)
   {
      std::string json;

      {
         test::SimpleVO value;
         value.m_long = 666;
         value.m_short = 42;
         value.m_string = "bla bla bla";

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

   /*
   TEST( casual_sf_json_archive, complex_write_read)
   {
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
   */

   TEST( casual_sf_json_archive, load_invalid_document__expecting_exception)
   {
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

      EXPECT_THROW
      ({
         sf::archive::json::Load{}( json);
      }, sf::exception::archive::invalid::Document);
   }

   TEST( casual_sf_json_archive, read_with_invalid_long__expecting_exception)
   {
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

      EXPECT_THROW
      ({
         test::SimpleVO value;
         local::string_to_strict_value( json, value);
      }, sf::exception::archive::invalid::Node);

   }

   TEST( casual_sf_json_archive, read_with_invalid_string__expecting_exception)
   {
      const std::string json
      {
         R"(
{
   "value":
   {
      "m_long": 123456,
      "m_string": false,
      "m_short": 23,
      "m_longlong": 1234567890123456789,
      "m_time": 1234567890
   }
}
)"
      };

      EXPECT_THROW
      ({
         test::SimpleVO value;
         local::string_to_strict_value( json, value);
      }, sf::exception::archive::invalid::Node);

   }


}


