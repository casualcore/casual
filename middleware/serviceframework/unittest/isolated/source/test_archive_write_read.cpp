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
#include "sf/archive/ini.h"
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
                  sf::archive::json::Save save;

                  sf::archive::json::Writer writer( save.target());

                  writer << CASUAL_MAKE_NVP( value);

                  save.serialize( data);

               }

               {
                  sf::archive::json::Load load;
                  load.serialize( data);

                  archive::json::basic_reader< policy_type> reader( load.source());
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

               archive::yaml::Save save;

               {
                  archive::yaml::Writer writer( save.target());
                  writer << CASUAL_MAKE_NVP( value);
               }

               std::string yaml;
               save.serialize( yaml);

               archive::yaml::Load load;
               load.serialize( yaml);

               {
                  sf::archive::yaml::basic_reader< policy_type> reader( load.source());
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
               archive::xml::Save save;

               {
                  archive::xml::Writer writer( save.target());
                  writer << CASUAL_MAKE_NVP( value);
               }

               std::string xml;
               save.serialize( xml);

               archive::xml::Load load;
               load.serialize( xml);

               {
                  archive::basic_reader< archive::xml::reader::Implementation, P> reader( load.source());
                  T value;
                  reader >> CASUAL_MAKE_NVP( value);

                  return value;
               }
            }
         };


         template< typename P>
         struct ini
         {
            using policy_type = P;

            template< typename T>
            static T write_read( const T& value)
            {
               archive::ini::Save save;

               {
                  archive::ini::Writer writer( save.target());
                  writer << CASUAL_MAKE_NVP( value);
               }

               std::string ini;
               save.serialize( ini);

               std::cout << ini << std::endl;

               archive::ini::Load load;
               load.serialize( ini);

               {
                  archive::basic_reader< archive::ini::reader::Implementation, P> reader( load.source());
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
            //holder::ini< archive::policy::Strict>,  // cannot handle nested containers yet
            //holder::ini< archive::policy::Relaxed>, // cannot handle nested containers yet
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

      TYPED_TEST( casual_sf_archive_write_read, type_string_with_new_line)
      {
         std::string value = "first\nother";
         EXPECT_TRUE( TestFixture::write_read( value) == "first\nother");
      }

      // TODO: gives warning from clang and gives failure on OSX with locale "UTF-8"
      TYPED_TEST( casual_sf_archive_write_read, DISABLED_type_extended_string)
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

