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

            template< typename T>
            static auto write_read( const T& from) -> decltype( from.total())
            {
               auto result = from.total();
               std::string data;

               {
                  sf::archive::json::Save save;
                  sf::archive::json::Writer writer( save());

                  writer << sf::makeNameValuePair( "value", from);

                  save( data);

               }

               {
                  sf::archive::json::Load load;
                  load( data);

                  archive::json::relaxed::Reader reader( load());
                  reader >> sf::makeNameValuePair( "value", result);
               }
               return result;
            }
         };

         struct yaml
         {
            template< typename T>
            static auto write_read( const T& from) -> decltype( from.total())
            {

               auto result = from.total();
               archive::yaml::Save save;

               {
                  archive::yaml::Writer writer( save());
                  writer << sf::makeNameValuePair( "value", from);
               }

               std::string yaml;
               save( yaml);

               archive::yaml::Load load;
               load( yaml);

               {
                  sf::archive::yaml::relaxed::Reader reader( load());
                  reader >> sf::makeNameValuePair( "value", result);
               }
               return result;
            }
         };

         struct xml
         {

            template< typename T>
            static auto write_read( const T& from) -> decltype( from.total())
            {
               auto result = from.total();
               archive::xml::Save save;

               {
                  archive::xml::Writer writer( save());
                  writer << sf::makeNameValuePair( "value", from);
               }

               std::string xml;
               save( xml);

               archive::xml::Load load;
               load( xml);

               {
                  archive::xml::relaxed::Reader reader( load());
                  reader >> sf::makeNameValuePair( "value", result);
               }
               return result;
            }
         };


      } // holder


      template <typename H>
      struct casual_sf_relaxed_archive_write_read : public ::testing::Test, public H
      {

      };


      typedef ::testing::Types<
            holder::json,
            holder::yaml,
            holder::xml
       > archive_types;

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
                     Total total()  const { return Total{};}

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
                     Total total()  const { return Total{};}

                     std::string some_string;

                     std::vector< simple::Reduced> some_set;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                         archive & CASUAL_MAKE_NVP( some_string);
                         archive & CASUAL_MAKE_NVP( some_set);
                     )
                  };


               } // medium

            } // vo


         } // <unnamed>
      } // local

      TYPED_TEST( casual_sf_relaxed_archive_write_read, simple_vo)
      {
         auto result = TestFixture::write_read( local::vo::simple::Reduced{ "test"});

         EXPECT_TRUE( result.some_string == "test") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_long == 42) << CASUAL_MAKE_NVP( result);
      }


      TYPED_TEST( casual_sf_relaxed_archive_write_read, medium_vo)
      {
         auto result = TestFixture::write_read( local::vo::medium::Reduced{ "test", { { "index-0"}, { "index-1"}}});

         EXPECT_TRUE( result.some_string == "test") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_long == 666) << CASUAL_MAKE_NVP( result);
         ASSERT_TRUE( result.some_set.size() == 2) << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_set.at( 0).some_string == "index-0") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_set.at( 0).some_long == 42) << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_set.at( 1).some_string == "index-1") << CASUAL_MAKE_NVP( result);
         EXPECT_TRUE( result.some_set.at( 1).some_long == 42) << CASUAL_MAKE_NVP( result);
      }


   } // sf

}

