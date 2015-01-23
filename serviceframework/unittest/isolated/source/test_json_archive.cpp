//!
//! test_json_archive.cpp
//!
//! Created on: Jul 10, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "sf/archive/json.h"

#include "../include/test_vo.h"

#include <json-c/json.h>


namespace casual
{

   namespace local
   {
      namespace
      {

         template< typename T>
         void value_to_string( T&& value, std::string& string)
         {
            sf::archive::json::Save target;
            sf::archive::json::Writer writer( target());

            writer << CASUAL_MAKE_NVP( value);

            target( string);
         }

         template< typename T>
         void string_to_strict_value( const std::string& string, T&& value)
         {
            sf::archive::json::Load source;
            source( string);

            sf::archive::json::Reader reader( source());

            reader >> CASUAL_MAKE_NVP( value);
         }

         template< typename T>
         void string_to_relaxed_value( const std::string& string, T&& value)
         {
            sf::archive::json::Load source;
            source( string);

            sf::archive::json::relaxed::Reader reader( source());

            reader >> CASUAL_MAKE_NVP( value);
         }

      }

   }


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

   TEST( casual_sf_json_archive, simple_write_read)
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



}


