//!
//! casual_isolatedunittest_yaml_archive.cpp
//!
//! Created on: Oct 31, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "../include/test_vo.h"

#include "sf/archive/yaml.h"
#include "sf/exception.h"


namespace casual
{
   /*
   namespace local
   {
      struct Serializible
      {
         long someLong;
         long long someLongLong;
         std::string someString;

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( someLong);
            archive & CASUAL_MAKE_NVP( someString);
            archive & CASUAL_MAKE_NVP( someLongLong);

         }

         static std::string yaml()
         {
            return R"(
value:
   someLong: 234
   someString: bla bla bla bla
   someLongLong: 1234567890123456789
)";
         }
      };


      struct Binary : public Serializible
      {
         common::binary_type binary;

         template< typename A>
         void serialize( A& archive)
         {
            Serializible::serialize( archive);
            archive & CASUAL_MAKE_NVP( binary);
         }
      };



   }
   */


   TEST( casual_sf_yaml_archive, relaxed_read_serializible)
   {
      std::istringstream stream( test::SimpleVO::yaml());

      sf::archive::yaml::relaxed::Reader reader( stream);

      test::SimpleVO value;

      reader >> CASUAL_MAKE_NVP( value);

      EXPECT_TRUE( value.m_long == 234) << "value.m_long: " << value.m_long;
      EXPECT_TRUE( value.m_string == "bla bla bla bla") << "value.m_long: " << value.m_long;

   }


   TEST( casual_sf_yaml_archive, strict_read_serializible__gives_ok)
   {
      std::istringstream stream( test::SimpleVO::yaml());

      sf::archive::yaml::Reader reader( stream);

      test::SimpleVO value;

      reader >> CASUAL_MAKE_NVP( value);

      EXPECT_TRUE( value.m_long == 234) << "value.someLong: " << value.m_long;
      EXPECT_TRUE( value.m_string == "bla bla bla bla") << "value.someLong: " << value.m_string;
      EXPECT_TRUE( value.m_longlong == 1234567890123456789) << "value.someLongLong: " << value.m_longlong;

   }

   TEST( casual_sf_yaml_archive, strict_read_not_in_document__gives_throws)
   {
      std::istringstream stream( test::SimpleVO::yaml());

      sf::archive::yaml::Reader reader( stream);

      test::SimpleVO wrongRoleName;

      EXPECT_THROW(
      {
         reader >> CASUAL_MAKE_NVP( wrongRoleName);
      }, sf::exception::Validation);

   }


   TEST( casual_sf_yaml_archive, write_read_vector_pod)
   {
      YAML::Emitter output;

      sf::archive::yaml::Writer writer( output);

      {
         std::vector< long> values = { 1, 2, 34, 45, 34, 34, 23};
         writer << CASUAL_MAKE_NVP( values);
      }

      std::istringstream stream( output.c_str());
      sf::archive::yaml::relaxed::Reader reader( stream);


      std::vector< long> values;
      reader >> CASUAL_MAKE_NVP( values);



      ASSERT_TRUE( values.size() == 7) << "size: " << values.size() << output.c_str();
      EXPECT_TRUE( values.at( 0) == 1);
      EXPECT_TRUE( values.at( 1) == 2);
      EXPECT_TRUE( values.at( 2) == 34) << "values.at( 2): " << values.at( 2);
      EXPECT_TRUE( values.at( 3) == 45);
      EXPECT_TRUE( values.at( 4) == 34);
      EXPECT_TRUE( values.at( 5) == 34);
      EXPECT_TRUE( values.at( 6) == 23);
   }

   TEST( casual_sf_yaml_archive, write_read_vector_serializible)
   {
      YAML::Emitter output;

      sf::archive::yaml::Writer writer( output);

      {
         std::vector< test::SimpleVO> values = { { 2342342, "one two three", 123 }, { 234234, "four five six", 456}};
         writer << CASUAL_MAKE_NVP( values);
      }

      std::istringstream stream( output.c_str());
      sf::archive::yaml::relaxed::Reader reader( stream);

      std::vector< test::SimpleVO> values;
      reader >> CASUAL_MAKE_NVP( values);

      ASSERT_TRUE( values.size() == 2);
      EXPECT_TRUE( values.at( 0).m_short == 123);
      EXPECT_TRUE( values.at( 0).m_string == "one two three");
      EXPECT_TRUE( values.at( 1).m_short == 456);
      EXPECT_TRUE( values.at( 1).m_string == "four five six");

   }



   TEST( casual_sf_yaml_archive, write_read_map_complex)
   {
      YAML::Emitter output;

      sf::archive::yaml::Writer writer( output);

      {
         std::map< long, test::Composite> values;
         values[ 10].m_string = "kalle";
         values[ 10].m_values = { { 11111, "one", 1}, { 22222, "two", 2}};
         values[ 2342].m_string = "Charlie";
         values[ 2342].m_values = { { 33333, "three", 3}, { 444444, "four", 4}};

         writer << CASUAL_MAKE_NVP( values);
      }


      std::istringstream stream( output.c_str());
      sf::archive::yaml::relaxed::Reader reader( stream);

      std::map< long, test::Composite> values;
      reader >> CASUAL_MAKE_NVP( values);


      ASSERT_TRUE( values.size() == 2) << output.c_str();
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

   TEST( casual_sf_yaml_archive, write_read_binary)
   {
      YAML::Emitter output;
      sf::archive::yaml::Writer writer( output);

      {
         test::Binary value;

         value.m_long = 23;
         value.m_string = "Charlie";
         value.m_binary = { 1, 2, 56, 57, 58 };

         writer << CASUAL_MAKE_NVP( value);
      }

      std::istringstream stream( output.c_str());
      sf::archive::yaml::relaxed::Reader reader( stream);

      test::Binary value;
      reader >> CASUAL_MAKE_NVP( value);



      ASSERT_TRUE( value.m_binary.size() == 5) << "size: " << value.m_binary.size() << output.c_str();
      EXPECT_TRUE( value.m_binary.at( 0) == 1);
      EXPECT_TRUE( value.m_binary.at( 1) == 2);
      EXPECT_TRUE( value.m_binary.at( 2) == 56) << "values.at( 2): " << value.m_binary.at( 2);
      EXPECT_TRUE( value.m_binary.at( 3) == 57);
      EXPECT_TRUE( value.m_binary.at( 4) == 58);
   }

}


