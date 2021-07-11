//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <gtest/gtest.h>


#include "common/serialize/ini.h"

#include "common/unittest.h"

#include <sstream>
#include <locale>

#include "../../include/test_vo.h"



namespace casual
{
   namespace common
   {
      namespace
      {
         namespace local
         {
            template<typename T>
            void value_to_string( T&& value, std::string& string)
            {
               auto writer = serialize::ini::writer();
               writer << CASUAL_NAMED_VALUE( value);
               writer.consume( string);
            }

            template<typename T>
            void string_to_value( const std::string& string, T& value)
            {
               auto reader = serialize::ini::strict::reader( string);
               reader >> CASUAL_NAMED_VALUE( value);
            }

         } // local
      } //

      TEST( common_serialize_ini_archive, writer_archive_type)
      {
         common::unittest::Trace trace;

         auto writer = serialize::ini::writer();

         EXPECT_TRUE( writer.archive_type() == common::serialize::archive::Type::dynamic_type) << "writer.archive_type: " << writer.archive_type();
         EXPECT_TRUE( writer.type() == common::serialize::archive::dynamic::Type::named) << "writer.type(): " << writer.type();
      }

      TEST( common_serialize_ini_archive, reader_archive_type)
      {
         common::unittest::Trace trace;

         std::string buffer;
         auto reader = serialize::ini::strict::reader( buffer);

         EXPECT_TRUE( reader.archive_type() == common::serialize::archive::Type::dynamic_type) << "reader.archive_type: " << reader.archive_type();
         EXPECT_TRUE( reader.type() == common::serialize::archive::dynamic::Type::named) << "reader.type(): " << reader.type();
      }

      TEST( common_serialize_ini_archive, write_read_string_with_new_line)
      {
         common::unittest::Trace trace;

         std::string ini;
         std::string source = "foo\nbar";
         local::value_to_string( source, ini);
         std::string target;
         local::string_to_value( ini, target);

         EXPECT_TRUE( source == target);
      }

      TEST( common_serialize_ini_archive, write_read_boolean)
      {
         common::unittest::Trace trace;

         std::string ini;
         local::value_to_string( true, ini);
         bool target = false;
         local::string_to_value( ini, target);
         EXPECT_TRUE( target == true);
      }

      TEST( common_serialize_ini_archive, write_read_decimal)
      {
         common::unittest::Trace trace;

         std::string ini;
         float source = 3.14;
         local::value_to_string( source, ini);
         float target = 0.0;
         local::string_to_value( ini, target);
         EXPECT_TRUE( source == target);
      }

      TEST( common_serialize_ini_archive, write_read_container)
      {
         common::unittest::Trace trace;

         std::string ini;
         std::vector<long> source{ 1, 3, 5, 7 };
         local::value_to_string( source, ini);
         std::vector<long> target;
         local::string_to_value( ini, target);
         EXPECT_TRUE( source == target);
      }

      namespace
      {
         struct SomeVO
         {
            struct FirstVO
            {
               long integer;
               std::string string;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( integer);
                  CASUAL_SERIALIZE( string);
               )
            };


            struct OtherVO
            {
               struct InnerVO
               {
                  long long huge;

                  CASUAL_CONST_CORRECT_SERIALIZE
                  (
                     CASUAL_SERIALIZE( huge);
                  )

               };

               InnerVO first_inner;
               InnerVO other_inner;


               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( first_inner);
                  CASUAL_SERIALIZE( other_inner);
               )
            };

            bool boolean;
            FirstVO first;
            short tiny;
            std::vector<OtherVO> others;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( boolean);
               CASUAL_SERIALIZE( first);
               CASUAL_SERIALIZE( tiny);
               CASUAL_SERIALIZE( others);
            )

         };

         TEST( common_serialize_ini_archive, write_read_serializable)
         {
            common::unittest::Trace trace;

            std::string ini;
            SomeVO source;
            source.boolean = true;
            source.first.integer = 654321;
            source.first.string = "foo";
            source.tiny = 123;
            {
               SomeVO::OtherVO other;
               other.first_inner.huge = 123456789;
               other.other_inner.huge = 987654321;
               source.others.push_back( other);
            }
            {
               SomeVO::OtherVO other;
               other.first_inner.huge = 13579;
               other.other_inner.huge = 97531;
               source.others.push_back( other);
            }


            local::value_to_string( source, ini);
            SomeVO target;
            local::string_to_value( ini, target);

            EXPECT_TRUE( source.boolean == target.boolean);
            EXPECT_TRUE( source.first.integer == target.first.integer);
            EXPECT_TRUE( source.first.string == target.first.string);
            EXPECT_TRUE( source.tiny == target.tiny);
            ASSERT_TRUE( target.others.size() == 2);
            EXPECT_TRUE( source.others.at( 0).first_inner.huge == target.others.at( 0).first_inner.huge);
            EXPECT_TRUE( source.others.at( 0).other_inner.huge == target.others.at( 0).other_inner.huge);
            EXPECT_TRUE( source.others.at( 1).first_inner.huge == target.others.at( 1).first_inner.huge);
            EXPECT_TRUE( source.others.at( 1).other_inner.huge == target.others.at( 1).other_inner.huge);
         }


         TEST( common_serialize_ini_archive, test_control_characters)
         {
            common::unittest::Trace trace;

            std::vector< char> result;
            for( short idx = 0; idx < 255; ++idx)
            {
               if( std::iscntrl( static_cast<char>( idx), std::locale{}))
               {
                  result.push_back( idx);
               }
            }

            auto expected = std::vector< char>{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,127};

            EXPECT_TRUE( result == expected);
         }
      }

   } // common
} // casual
