//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <gtest/gtest.h>


#include "sf/namevaluepair.h"
#include "sf/archive/json.h"
#include "sf/archive/yaml.h"
#include "sf/archive/xml.h"
#include "sf/log.h"

#include "common/log.h"



#include <string>
#include <vector>



namespace casual
{
   namespace sf
   {

      namespace holder
      {
         struct json
         {

            template< typename To, typename From>
            static To write_read( const From& from)
            {
               std::string data;

               {
                  auto writer = sf::archive::json::writer( data);
                  writer << sf::name::value::pair::make( "value", from);
               }

               To result;
               {
                  auto reader = sf::archive::json::relaxed::reader( data);
                  reader >> sf::name::value::pair::make( "value", result);
               }
               return result;
            }
         };

         struct yaml
         {
            template< typename To, typename From>
            static To write_read( const From& from)
            {
               std::string yaml;

               {
                  auto writer = archive::yaml::writer( yaml);
                  writer << sf::name::value::pair::make( "value", from);
               }

               To result;
               {
                  auto reader = archive::yaml::relaxed::reader( yaml);
                  reader >> sf::name::value::pair::make( "value", result);
               }
               return result;
            }
         };

         struct xml
         {

            template< typename To, typename From>
            static To write_read( const From& from)
            {
               std::string xml;

               {
                  auto writer = archive::xml::writer( xml);
                  writer << sf::name::value::pair::make( "value", from);
               }

               To result;
               {
                  auto reader = archive::xml::relaxed::reader( xml);
                  reader >> sf::name::value::pair::make( "value", result);
               }
               return result;
            }
         };


      } // holder


      template <typename H>
      struct casual_sf_relaxed_archive_write_read : public ::testing::Test, public H
      {

      };


      using archive_types = ::testing::Types<
            holder::yaml,
            holder::json,
            holder::xml
       >;

      TYPED_TEST_CASE(casual_sf_relaxed_archive_write_read, archive_types);


      namespace local
      {
         namespace
         {
            namespace vo
            {
               namespace simple
               {
                  struct Total
                  {
                     long some_long = 42;
                     std::string some_string;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        archive & CASUAL_MAKE_NVP( some_long);
                        archive & CASUAL_MAKE_NVP( some_string);
                     )
                  };

                  struct Reduced
                  {
                     std::string some_string;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                         archive & CASUAL_MAKE_NVP( some_string);
                     )
                  };
               } // simple

               namespace medium
               {
                  struct Total
                  {
                     long some_long = 666;
                     std::string some_string;

                     std::vector< simple::Total> some_set;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        archive & CASUAL_MAKE_NVP( some_long);
                        archive & CASUAL_MAKE_NVP( some_string);
                        archive & CASUAL_MAKE_NVP( some_set);
                     )
                  };

                  struct Reduced
                  {
                     std::string some_string;

                     std::vector< simple::Reduced> some_set;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                         archive & CASUAL_MAKE_NVP( some_string);
                         archive & CASUAL_MAKE_NVP( some_set);
                     )
                  };


               } // medium

               template< typename T>
               struct Optional
               {
                  Optional() = default;
                  Optional( T value) : optional_value( value) {}

                  // we need to have at least one value for yaml and json to work
                  long dummy_value = 0;
                  sf::optional< T> optional_value;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                      archive & CASUAL_MAKE_NVP( dummy_value);
                      archive & CASUAL_MAKE_NVP( optional_value);
                  )
               };

            } // vo


         } // <unnamed>
      } // local

      TYPED_TEST( casual_sf_relaxed_archive_write_read, simple_vo)
      {
         auto result = TestFixture::template write_read< local::vo::simple::Total>( local::vo::simple::Reduced{ "test test"});

         EXPECT_TRUE( result.some_string == "test test") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_long == 42) << CASUAL_MAKE_NVP( result);
      }


      TYPED_TEST( casual_sf_relaxed_archive_write_read, medium_vo)
      {
         auto result = TestFixture::template write_read< local::vo::medium::Total>( local::vo::medium::Reduced{ "test", { { "index-0"}, { "index-1"}}});

         EXPECT_TRUE( result.some_string == "test") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_long == 666) << CASUAL_MAKE_NVP( result);
         ASSERT_TRUE( result.some_set.size() == 2) << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_set.at( 0).some_string == "index-0") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_set.at( 0).some_long == 42) << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_set.at( 1).some_string == "index-1") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_set.at( 1).some_long == 42) << CASUAL_MAKE_NVP( result);
      }

      TYPED_TEST( casual_sf_relaxed_archive_write_read, optional_empty)
      {
         auto result = TestFixture::template write_read< local::vo::Optional< int>>( local::vo::Optional< int>{});

         EXPECT_TRUE( ! result.optional_value.has_value()) << CASUAL_MAKE_NVP( result);
      }

      TYPED_TEST( casual_sf_relaxed_archive_write_read, optional_has_value)
      {
         auto result = TestFixture::template write_read< local::vo::Optional< std::size_t>>( local::vo::Optional< std::size_t>{ 42l});

         EXPECT_TRUE( result.optional_value == 42ul) << CASUAL_MAKE_NVP( result);
      }


      TYPED_TEST( casual_sf_relaxed_archive_write_read, optional_empty_vector)
      {
         using optional_type = local::vo::Optional< std::vector< int>>;

         auto result = TestFixture::template write_read< optional_type>( optional_type{});

         EXPECT_TRUE( ! result.optional_value.has_value()) << CASUAL_MAKE_NVP( result);
      }

      TYPED_TEST( casual_sf_relaxed_archive_write_read, optional_has_value_vector)
      {
         using optional_type = local::vo::Optional< std::vector< int>>;

         std::vector< int> value{ 1, 2, 3, 4, 5, 6};

         auto result = TestFixture::template write_read< optional_type>( optional_type{ value});

         EXPECT_TRUE( result.optional_value == value) << CASUAL_MAKE_NVP( result);
      }

      TYPED_TEST( casual_sf_relaxed_archive_write_read, optional_medium)
      {
         auto result = TestFixture::template write_read<
               local::vo::Optional< local::vo::medium::Total>>(
                     local::vo::Optional< local::vo::medium::Reduced>(
                           local::vo::medium::Reduced{ "test test", { { "index-0"}, { "index-1"}}}));

         EXPECT_TRUE( result.optional_value.has_value()) << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.optional_value.value().some_string == "test test") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.optional_value.value().some_long == 666) << CASUAL_MAKE_NVP( result);
         ASSERT_TRUE( result.optional_value.value().some_set.size() == 2) << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.optional_value.value().some_set.at( 0).some_string == "index-0") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.optional_value.value().some_set.at( 0).some_long == 42) << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.optional_value.value().some_set.at( 1).some_string == "index-1") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.optional_value.value().some_set.at( 1).some_long == 42) << CASUAL_MAKE_NVP( result);

      }


   } // sf

}

