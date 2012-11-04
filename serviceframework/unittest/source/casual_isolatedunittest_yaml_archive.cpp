//!
//! casual_isolatedunittest_yaml_archive.cpp
//!
//! Created on: Oct 31, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include <yaml-cpp/yaml.h>

#include "casual_archive_yaml_policy.h"



namespace casual
{
   namespace local
   {
      struct Serializible
      {
         long someLong;
         std::string someString;

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( someLong);
            archive & CASUAL_MAKE_NVP( someString);

         }

         static std::string yaml()
         {
            std::string result = "value:\n";
            result += "   someLong: 234\n";
            result += "   someString: bla bla bla bla\n";

            return result;
         }
      };




   }


   TEST( casual_sf_yaml_archive, read_serializible)
   {
      std::istringstream stream( local::Serializible::yaml());

      sf::archive::YamlReader reader( stream);

      local::Serializible value;

      reader >> CASUAL_MAKE_NVP( value);

      EXPECT_TRUE( value.someLong == 234) << "value.someLong: " << value.someLong;
      EXPECT_TRUE( value.someString == "bla bla bla bla") << "value.someLong: " << value.someString;

   }




   TEST( casual_sf_yaml_archive, write_read_vector_pod)
   {
      YAML::Emitter output;

      sf::archive::YamlWriter writer( output);

      {
         std::vector< long> values = { 1, 2, 34, 45, 34, 34, 23};
         writer << CASUAL_MAKE_NVP( values);
      }

      std::istringstream stream( output.c_str());
      sf::archive::YamlReader reader( stream);


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

      sf::archive::YamlWriter writer( output);

      {
         std::vector< local::Serializible> values = { { 123, "one two three"}, { 456, "four five six"}};
         writer << CASUAL_MAKE_NVP( values);
      }

      std::istringstream stream( output.c_str());
      sf::archive::YamlReader reader( stream);

      std::vector< local::Serializible> values;
      reader >> CASUAL_MAKE_NVP( values);

      ASSERT_TRUE( values.size() == 2);
      EXPECT_TRUE( values.at( 0).someLong == 123);
      EXPECT_TRUE( values.at( 0).someString == "one two three");
      EXPECT_TRUE( values.at( 1).someLong == 456);
      EXPECT_TRUE( values.at( 1).someString == "four five six");

   }

   namespace local
   {
      struct Composite
      {
         std::string someString;
         std::vector< Serializible> someValues;

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( someString);
            archive & CASUAL_MAKE_NVP( someValues);

         }
      };

   }

   TEST( casual_sf_yaml_archive, write_read_map_complex)
   {
      YAML::Emitter output;

      sf::archive::YamlWriter writer( output);

      {
         std::map< long, local::Composite> values;
         values[ 10].someString = "kalle";
         values[ 10].someValues = { { 1, "one"}, { 2, "two"}};
         values[ 2342].someString = "Charlie";
         values[ 2342].someValues = { { 3, "three"}, { 4, "four"}};

         writer << CASUAL_MAKE_NVP( values);
      }


      std::istringstream stream( output.c_str());
      sf::archive::YamlReader reader( stream);

      std::map< long, local::Composite> values;
      reader >> CASUAL_MAKE_NVP( values);


      ASSERT_TRUE( values.size() == 2) << output.c_str();
      EXPECT_TRUE( values.at( 10).someString == "kalle");
      EXPECT_TRUE( values.at( 10).someValues.at( 0).someLong == 1);
      EXPECT_TRUE( values.at( 10).someValues.at( 0).someString == "one");
      EXPECT_TRUE( values.at( 10).someValues.at( 1).someLong == 2);
      EXPECT_TRUE( values.at( 10).someValues.at( 1).someString == "two");

      EXPECT_TRUE( values.at( 2342).someString == "Charlie");
      EXPECT_TRUE( values.at( 2342).someValues.at( 0).someLong == 3);
      EXPECT_TRUE( values.at( 2342).someValues.at( 0).someString == "three");
      EXPECT_TRUE( values.at( 2342).someValues.at( 1).someLong == 4);
      EXPECT_TRUE( values.at( 2342).someValues.at( 1).someString == "four");



   }
}


