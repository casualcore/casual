//!
//! casual_isolatedunittest_archive.cpp
//!
//! Created on: Oct 14, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>


#include "sf/namevaluepair.h"
#include "sf/basic_archive.h"
#include "sf/archive_binary.h"

#include "../include/test_vo.h"


#include <string>

#include <typeinfo>



namespace casual
{


   TEST( casual_sf_binary_writer, serialize_pod)
   {

      sf::buffer::Binary buffer;
      sf::archive::binary::Writer writer( buffer);

      writer << CASUAL_MAKE_NVP( 10);
   }

   TEST( casual_sf_binary_writer, serialize_string)
   {

      sf::buffer::Binary buffer;
      sf::archive::binary::Writer writer( buffer);

      writer << CASUAL_MAKE_NVP( std::string{ "test"});

      sf::archive::binary::Reader reader( buffer);

      std::string result;

      reader >> CASUAL_MAKE_NVP( result);

      EXPECT_TRUE( result == "test") << "result: " << result;



   }


   TEST( casual_sf_binary_reader_writer, serialize_pod)
   {

      sf::buffer::Binary buffer;
      sf::archive::binary::Writer writer( buffer);

      writer << CASUAL_MAKE_NVP( 34L);

      sf::archive::binary::Reader reader( buffer);

      long result;

      reader >> CASUAL_MAKE_NVP( result);

      EXPECT_TRUE( result == 34) << "result: " << result;

   }



   TEST( casual_sf_binary_reader_writer, serialize_vector_long)
   {

      sf::buffer::Binary buffer;
      sf::archive::binary::Writer writer( buffer);


      std::vector< long> someInts = { 1, 2, 3, 4 };

      writer << CASUAL_MAKE_NVP( someInts);

      std::vector< long> result;

      sf::archive::binary::Reader reader( buffer);

      reader >> CASUAL_MAKE_NVP( result);

      ASSERT_TRUE( result.size() == 4) << "result.size(): " << result.size();
      EXPECT_TRUE( result.at( 0) == 1);
      EXPECT_TRUE( result.at( 1) == 2);
      EXPECT_TRUE( result.at( 2) == 3);
      EXPECT_TRUE( result.at( 3) == 4);

   }


   TEST( casual_sf_binary_reader_writer, map_long_string)
   {

      sf::buffer::Binary buffer;
      sf::archive::binary::Writer writer( buffer);

      std::map< long, std::string> value = { { 1, "test 1"}, { 2, "test 2"}, { 3, "test 3"}, { 4, "test 4"} };

      writer << CASUAL_MAKE_NVP( value);


      std::map< long, std::string> result;

      sf::archive::binary::Reader reader( buffer);

      reader >> CASUAL_MAKE_NVP( result);

      ASSERT_TRUE( result.size() == 4) << "result.size(): " << result.size();
      EXPECT_TRUE( result.at( 1) == "test 1") << "result.at( 1): " << result.at( 1);
      EXPECT_TRUE( result.at( 2) == "test 2");
      EXPECT_TRUE( result.at( 3) == "test 3");
      EXPECT_TRUE( result.at( 4) == "test 4");


   }

   struct Serializible
   {


      std::string someString;
      long someLong;

      template< typename A>
      void serialize( A& archive)
      {
         archive & CASUAL_MAKE_NVP( someString);
         archive & CASUAL_MAKE_NVP( someLong);
      }
   };

   TEST( casual_sf_binary_reader_writer, serializible)
   {

      sf::buffer::Binary buffer;

      {
         sf::archive::binary::Writer writer( buffer);

         Serializible value;
         value.someLong = 23;
         value.someString = "kdjlfskjf";

         writer << CASUAL_MAKE_NVP( value);
      }

      {
         sf::archive::binary::Reader reader( buffer);

         Serializible value;
         reader >> CASUAL_MAKE_NVP( value);

         EXPECT_TRUE( value.someLong == 23);
         EXPECT_TRUE( value.someString == "kdjlfskjf");
      }

   }

   TEST( casual_sf_binary_reader_writer, complex_serializible)
   {

      sf::buffer::Binary buffer;

      {
         test::Composite value;
         value.m_string = "test";
         value.m_values = { 1, 2, 3, 4};

         std::vector< test::Composite> range{ value, value, value, value};

         sf::archive::binary::Writer writer( buffer);
         writer << CASUAL_MAKE_NVP( range);
      }

      {
         sf::archive::binary::Reader reader( buffer);

         std::vector< test::Composite> range;
         reader >> CASUAL_MAKE_NVP( range);

         ASSERT_TRUE( range.size() == 4);
         ASSERT_TRUE( range.at( 0).m_values.size() == 4);
         EXPECT_TRUE( range.at( 0).m_values.at( 0).m_long == 1);
         EXPECT_TRUE( range.at( 0).m_values.at( 1).m_long == 2);
         EXPECT_TRUE( range.at( 0).m_values.at( 2).m_long == 3);
         EXPECT_TRUE( range.at( 0).m_values.at( 3).m_long == 4);
      }

   }


}

