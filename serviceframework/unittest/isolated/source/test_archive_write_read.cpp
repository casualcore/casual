//!
//! casual_isolatedunittest_archive.cpp
//!
//! Created on: Oct 14, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>


#include "sf/namevaluepair.h"
#include "sf/archive/json.h"
#include "sf/archive/yaml.h"
#include "sf/archive/xml.h"
#include "sf/archive/binary.h"

#include "common/log.h"

#include "../include/test_vo.h"


#include <string>
#include <random>
#include <set>
#include <vector>
#include <deque>


namespace casual
{
   namespace sf
   {

      namespace holder
      {
         template< typename P>
         struct json
         {
            using policy_type = P;

            template< typename T>
            static T write_read( const T& value)
            {
               std::string data;
               {
                  json_object* root = nullptr;
                  sf::archive::json::Writer writer( root);

                  writer << CASUAL_MAKE_NVP( value);
                  data = json_object_to_json_string( root);
               }
               {
                  //std::cerr << "json data: " << data << std::endl;
                  archive::json::basic_reader< policy_type> reader( data.c_str());
                  T value;
                  reader >> CASUAL_MAKE_NVP( value);
                  return value;
               }
            }
         };

         template< typename P>
         struct yaml
         {
            using policy_type = P;

            template< typename T>
            static T write_read( const T& value)
            {
               YAML::Emitter output;

               {
                  archive::yaml::Writer writer( output);
                  writer << CASUAL_MAKE_NVP( value);
               }

               {
                  std::istringstream stream( output.c_str());
                  sf::archive::yaml::basic_reader< policy_type> reader( stream);
                  T value;
                  reader >> CASUAL_MAKE_NVP( value);
                  return value;
               }
            }
         };

         template< typename P>
         struct xml
         {
            using policy_type = P;

            template< typename T>
            static T write_read( const T& value)
            {
               archive::xml::target_type output;

               {
                  archive::xml::Writer writer( output);
                  writer << CASUAL_MAKE_NVP( value);
               }

               //
               // TODO: Perhaps serialize to neutral format first ?
               //

               {
                  archive::basic_reader< archive::xml::reader::Implementation, P> reader( output);
                  T value;
                  reader >> CASUAL_MAKE_NVP( value);

                  return value;
               }
            }
         };



         struct binary
         {
            template< typename T>
            static T write_read( const T& value)
            {
               sf::buffer::binary::Stream buffer;
               {
                  sf::archive::binary::Writer writer( buffer);
                  writer << CASUAL_MAKE_NVP( value);
               }

               {
                  sf::archive::binary::Reader reader( buffer);
                  T value;
                  reader >> CASUAL_MAKE_NVP( value);
                  return value;
               }
            }
         };

      } // holder


      template <typename H>
      struct casual_sf_archive_write_read : public ::testing::Test, public H
      {

      };


      typedef ::testing::Types<
            holder::json< archive::policy::Strict>,
            holder::json< archive::policy::Relaxed>,
            holder::yaml< archive::policy::Strict>,
            holder::yaml< archive::policy::Relaxed>,
            holder::xml< archive::policy::Strict>,
            holder::xml< archive::policy::Relaxed>,
            holder::binary
       > archive_types;

      TYPED_TEST_CASE(casual_sf_archive_write_read, archive_types);


      template< typename F, typename T>
      void test_value_min_max( T value)
      {
         EXPECT_TRUE( F::write_read( value) == value);
         {
            //std::cerr << std::fixed << "max: " << std::numeric_limits< T>::max() << std::endl;
            T value = std::numeric_limits< T>::max() - 1;
            EXPECT_TRUE( F::write_read( value) == value);
         }
         {
            T value = std::numeric_limits< T>::min() + 1;
            EXPECT_TRUE( F::write_read( value) == value);
         }
      }

      TYPED_TEST( casual_sf_archive_write_read, type_bool)
      {
         bool value = true;
         //test_value_min_max< TestFixture>( value);
         EXPECT_TRUE( TestFixture::write_read( value) == true) << TestFixture::write_read( value);
      }


      TYPED_TEST( casual_sf_archive_write_read, type_char)
      {
         char value = 'A';
         //test_value_min_max< TestFixture>( value);
         EXPECT_TRUE( TestFixture::write_read( value) == 'A') << TestFixture::write_read( value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_short)
      {
         short value = 42;
         test_value_min_max< TestFixture>( value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_int)
      {
         int value = 42;
         test_value_min_max< TestFixture>( value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_long)
      {
         long value = 42;
         test_value_min_max< TestFixture>( value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_string)
      {
         std::string value = "value 42";
         EXPECT_TRUE( TestFixture::write_read( value) == "value 42");
      }

      TYPED_TEST( casual_sf_archive_write_read, type_extended_string)
      {
         std::string value = u8"Bängen Trålar";
         EXPECT_TRUE( TestFixture::write_read( value) == u8"Bängen Trålar");
      }

      TYPED_TEST( casual_sf_archive_write_read, type_double)
      {
         double value = 42.42;
         //test_value_min_max< TestFixture>( value);
         EXPECT_TRUE( TestFixture::write_read( value) == value) << TestFixture::write_read( value);
      }


      TYPED_TEST( casual_sf_archive_write_read, type_binary)
      {
         common::platform::binary_type value{ 0, 42, -123, 23, 43, 11, 124};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_vector_long)
      {
         std::vector< long> value{ 234, 34234, 3242, 4564, 6456, 546, 3453, 78678, 35345};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_list_long)
      {
         std::list< long> value{ 234, 34234, 3242, 4564, 6456, 546, 3453, 78678, 35345};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_deque_long)
      {
         std::deque< long> value{ 234, 34234, 3242, 4564, 6456, 546, 3453, 78678, 35345};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_set_long)
      {
         std::set< long> value{ 234, 34234, 3242, 4564, 6456, 546, 3453, 78678, 35345};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_map_long_string)
      {
         std::map< long, std::string> value{ { 234, "poo"}, { 34234, "sdkfljs"}, { 3242, "cmx,nvxnvjkjdf"}};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_vector_vector_long)
      {
         std::mt19937 random{ std::random_device{}()};
         std::vector< std::vector< long>> value( 10);
         for( auto& values : value)
         {
            values = { 234, 34234, 3242, 4564, 6456, 546, 3453, 78678, 35345};
            std::shuffle( std::begin( values), std::end( values), random);
         }
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_tuple)
      {
         std::tuple< int, std::string, char, long, float> value{ 23, "charlie", 'Q', 343534323434, 1.42};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( casual_sf_archive_write_read, type_vector_tuple)
      {
         using tuple_type = std::tuple< int, std::string, char, long, float>;
         tuple_type tuple{ 23, "charlie", 'Q', 343534323434, 1.42};

         std::vector< tuple_type> value{
            tuple, tuple, tuple, tuple
         };
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }


   } // sf

}

