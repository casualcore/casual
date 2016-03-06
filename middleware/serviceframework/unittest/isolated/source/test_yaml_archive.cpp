//!
//! casual_isolatedunittest_yaml_archive.cpp
//!
//! Created on: Oct 31, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "../include/test_vo.h"

#include "sf/archive/yaml.h"
#include "sf/exception.h"
#include "sf/log.h"


#include "common/algorithm.h"


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

   namespace local
   {
      namespace
      {
         template<typename T>
         void string_to_strict_value( const std::string& string, T&& value)
         {
            sf::archive::yaml::Load load;
            load.serialize( string);
            sf::archive::yaml::Reader reader( load.source());
            reader >> CASUAL_MAKE_NVP( value);
         }

         template<typename T>
         void string_to_relaxed_value( const std::string& string, T&& value)
         {
            sf::archive::yaml::Load load;
            load.serialize( string);
            sf::archive::yaml::relaxed::Reader reader( load.source());
            reader >> CASUAL_MAKE_NVP( value);
         }

         template<typename T>
         void value_to_string( T&& value, std::string& string)
         {
            sf::archive::yaml::Save save;
            sf::archive::yaml::Writer writer( save.target());
            writer << CASUAL_MAKE_NVP( value);
            save.serialize( string);
         }

      }
   }


   TEST( casual_sf_yaml_archive, relaxed_read_serializible)
   {
      test::SimpleVO value;

      local::string_to_relaxed_value( test::SimpleVO::yaml(), value);

      EXPECT_TRUE( value.m_long == 234) << "value.m_long: " << value.m_long;
      EXPECT_TRUE( value.m_string == "bla bla bla bla") << "value.m_long: " << value.m_long;

   }


   TEST( casual_sf_yaml_archive, strict_read_serializible__gives_ok)
   {
      test::SimpleVO value;

      local::string_to_strict_value( test::SimpleVO::yaml(), value);

      EXPECT_TRUE( value.m_long == 234) << "value.someLong: " << value.m_long;
      EXPECT_TRUE( value.m_string == "bla bla bla bla") << "value.someLong: " << value.m_string;
      EXPECT_TRUE( value.m_longlong == 1234567890123456789) << "value.someLongLong: " << value.m_longlong;
   }

   TEST( casual_sf_yaml_archive, strict_read_not_in_document__gives_throws)
   {
      sf::archive::yaml::Load load;
      load.serialize( test::SimpleVO::yaml());
      sf::archive::yaml::Reader reader( load.source());

      test::SimpleVO wrongRoleName;

      EXPECT_THROW(
      {
         reader >> CASUAL_MAKE_NVP( wrongRoleName);
      }, sf::exception::archive::invalid::Node);

   }


   TEST( casual_sf_yaml_archive, write_read_vector_pod)
   {
      std::string yaml;

      {
         std::vector< long> values = { 1, 2, 34, 45, 34, 34, 23};
         local::value_to_string( values, yaml);
      }



      std::vector< long> values;
      local::string_to_relaxed_value( yaml, values);


      ASSERT_TRUE( values.size() == 7) << values.size();
      EXPECT_TRUE( values.at( 0) == 1);
      EXPECT_TRUE( values.at( 1) == 2) << common::range::make( values);
      EXPECT_TRUE( values.at( 2) == 34) << values.at( 2);
      EXPECT_TRUE( values.at( 3) == 45);
      EXPECT_TRUE( values.at( 4) == 34);
      EXPECT_TRUE( values.at( 5) == 34);
      EXPECT_TRUE( values.at( 6) == 23);
   }

   TEST( casual_sf_yaml_archive, write_read_vector_serializible)
   {
      std::string yaml;

      {
         std::vector< test::SimpleVO> values = { { 2342342, "one two three", 123 }, { 234234, "four five six", 456}};
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



   TEST( casual_sf_yaml_archive, write_read_map_complex)
   {
      std::string yaml;

      {
         std::map< long, test::Composite> values;
         values[ 10].m_string = "kalle";
         values[ 10].m_values = { { 11111, "one", 1}, { 22222, "two", 2}};
         values[ 2342].m_string = "Charlie";
         values[ 2342].m_values = { { 33333, "three", 3}, { 444444, "four", 4}};

         local::value_to_string( values, yaml);
      }

      std::map< long, test::Composite> values;
      local::string_to_relaxed_value( yaml, values);


      ASSERT_TRUE( values.size() == 2) << CASUAL_MAKE_NVP( values);
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

      std::string yaml;

      {
         test::Binary value;

         value.m_long = 23;
         value.m_string = "Charlie";
         value.m_binary = { 1, 2, 56, 57, 58 };

         local::value_to_string( value, yaml);
      }

      test::Binary value;
      local::string_to_relaxed_value( yaml, value);



      ASSERT_TRUE( value.m_binary.size() == 5) << value.m_binary.size();
      EXPECT_TRUE( value.m_binary.at( 0) == 1);
      EXPECT_TRUE( value.m_binary.at( 1) == 2);
      EXPECT_TRUE( value.m_binary.at( 2) == 56) << value.m_binary.at( 2);
      EXPECT_TRUE( value.m_binary.at( 3) == 57);
      EXPECT_TRUE( value.m_binary.at( 4) == 58);
   }


   TEST( casual_sf_yaml_archive, read_invalid_document__expecting_exception)
   {
      const std::string yaml{ "   " };

      EXPECT_THROW
      ({
         sf::archive::yaml::Load().serialize( yaml);
      }, sf::exception::archive::invalid::Document);

   }

   TEST( casual_sf_yaml_archive, read_invalid_bool__expecting_exception)
   {
      static std::string yaml
      {
         R"(
value:
   m_bool: jajjemensan
   m_long: 123
   m_string: bla bla bla bla
   m_short: 23
   m_longlong: 1234567890123456789
   m_time: 1234567890
)"
      };

      EXPECT_THROW
      ({
         test::SimpleVO value;
         local::string_to_strict_value( yaml, value);
      }, sf::exception::archive::invalid::Node);
   }


} // casual


