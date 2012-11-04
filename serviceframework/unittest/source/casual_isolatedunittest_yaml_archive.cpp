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




   TEST( casual_sf_yaml_archive, vector_pod)
   {
      YAML::Emitter output;

      sf::archive::YamlWriter writer( output);

      std::vector< long> value = { 1, 2, 34, 45, 34, 34, 23};

      writer << CASUAL_MAKE_NVP( value);

      output << YAML::EndMap;

      EXPECT_TRUE( false) << "output: " << output.c_str();

   }

   TEST( casual_sf_yaml_archive, vector_serializible)
   {
      YAML::Emitter output;

      sf::archive::YamlWriter writer( output);

      std::vector< local::Serializible> value = { { 324, "sklfjslkf"}, { 234, "jl fsjsl flsjf skljdf"}};

      writer << CASUAL_MAKE_NVP( value);

      //output << YAML::EndMap;



      EXPECT_TRUE( false) << "output: " << output.c_str();

   }

   namespace local
   {
      struct Composite
      {
         Composite() : someValues( { { 324, "sklfjslkf"}, { 234, "jl fsjsl flsjf skljdf"}}) {}

         std::string someString;
         std::vector< Serializible> someValues;

         template< typename A>
         void serialize( A& archive)
         {
            archive << CASUAL_MAKE_NVP( someString);
            archive << CASUAL_MAKE_NVP( someValues);

         }
      };

   }

   TEST( casual_sf_yaml_archive, map_complex)
   {
      YAML::Emitter output;

      sf::archive::YamlWriter writer( output);

      std::map< long, local::Composite> value;
      value[ 10].someString = "kalle";
      value[ 2342].someString = "Charlie";


      writer << CASUAL_MAKE_NVP( value);

      //output << YAML::EndMap;



      EXPECT_TRUE( false) << "output: " << output.c_str();

   }
}


