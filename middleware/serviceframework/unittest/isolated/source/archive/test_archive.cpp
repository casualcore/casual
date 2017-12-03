//!
//! casual
//!


#include <gtest/gtest.h>


#include "sf/namevaluepair.h"
#include "sf/archive/binary.h"

#include "common/log.h"

#include "../../include/test_vo.h"


#include <string>

#include <typeinfo>



namespace casual
{


   TEST( casual_sf_binary_writer, serialize_pod)
   {

      sf::platform::binary::type buffer;
      auto writer = sf::archive::binary::writer( buffer);

      writer << CASUAL_MAKE_NVP( 10);
   }

   TEST( casual_sf_binary_writer, serialize_string)
   {

      sf::platform::binary::type buffer;
      auto writer = sf::archive::binary::writer( buffer);

      writer << CASUAL_MAKE_NVP( std::string{ "test"});

      auto reader = sf::archive::binary::reader( buffer);

      std::string result;

      reader >> CASUAL_MAKE_NVP( result);

      EXPECT_TRUE( result == "test") << "result: " << result;



   }


   TEST( casual_sf_binary_reader_writer, serialize_pod)
   {

      sf::platform::binary::type buffer;
      auto writer = sf::archive::binary::writer( buffer);

      writer << CASUAL_MAKE_NVP( 34L);

      auto reader = sf::archive::binary::reader( buffer);

      long result;

      reader >> CASUAL_MAKE_NVP( result);

      EXPECT_TRUE( result == 34) << "result: " << result;

   }



   TEST( casual_sf_binary_reader_writer, serialize_vector_long)
   {

      sf::platform::binary::type buffer;
      auto writer = sf::archive::binary::writer( buffer);


      std::vector< long> someInts = { 1, 2, 3, 4 };

      writer << CASUAL_MAKE_NVP( someInts);

      std::vector< long> result;

      auto reader = sf::archive::binary::reader( buffer);

      reader >> CASUAL_MAKE_NVP( result);

      ASSERT_TRUE( result.size() == 4) << "result.size(): " << result.size();
      EXPECT_TRUE( result.at( 0) == 1);
      EXPECT_TRUE( result.at( 1) == 2);
      EXPECT_TRUE( result.at( 2) == 3);
      EXPECT_TRUE( result.at( 3) == 4);

   }


   TEST( casual_sf_binary_reader_writer, map_long_string)
   {

      sf::platform::binary::type buffer;
      auto writer = sf::archive::binary::writer( buffer);

      std::map< long, std::string> value = { { 1, "test 1"}, { 2, "test 2"}, { 3, "test 3"}, { 4, "test 4"} };

      writer << CASUAL_MAKE_NVP( value);


      std::map< long, std::string> result;

      auto reader = sf::archive::binary::reader( buffer);

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

      CASUAL_CONST_CORRECT_SERIALIZE
      (
         archive & CASUAL_MAKE_NVP( someString);
         archive & CASUAL_MAKE_NVP( someLong);
      )
   };

   TEST( casual_sf_binary_reader_writer, serializible)
   {

      sf::platform::binary::type buffer;

      {
         auto writer = sf::archive::binary::writer( buffer);

         Serializible value;
         value.someLong = 23;
         value.someString = "kdjlfskjf";

         writer << CASUAL_MAKE_NVP( value);
      }

      {
         auto reader = sf::archive::binary::reader( buffer);

         Serializible value;
         reader >> CASUAL_MAKE_NVP( value);

         EXPECT_TRUE( value.someLong == 23);
         EXPECT_TRUE( value.someString == "kdjlfskjf");
      }

   }

   TEST( casual_sf_binary_reader_writer, complex_serializible)
   {

      sf::platform::binary::type buffer;

      {
         test::Composite value;
         value.m_string = "test";
         value.m_values = { 1, 2, 3, 4};
         std::get< 0>( value.m_tuple) = 10;
         std::get< 1>( value.m_tuple) = "poop";
         std::get< 2>( value.m_tuple).m_short = 42;


         std::vector< test::Composite> range{ value, value, value, value};

         auto writer = sf::archive::binary::writer( buffer);
         writer << CASUAL_MAKE_NVP( range);

      }

      {
         auto reader = sf::archive::binary::reader( buffer);

         std::vector< test::Composite> range;
         reader >> CASUAL_MAKE_NVP( range);

         ASSERT_TRUE( range.size() == 4);
         ASSERT_TRUE( range.at( 0).m_values.size() == 4);
         EXPECT_TRUE( range.at( 0).m_values.at( 0).m_long == 1);
         EXPECT_TRUE( range.at( 0).m_values.at( 1).m_long == 2);
         EXPECT_TRUE( range.at( 0).m_values.at( 2).m_long == 3);
         EXPECT_TRUE( range.at( 0).m_values.at( 3).m_long == 4);
         EXPECT_TRUE( std::get< 2>( range.at( 0).m_tuple).m_short == 42);
         EXPECT_TRUE( std::get< 2>( range.at( 3).m_tuple).m_short == 42);
      }

   }


}

