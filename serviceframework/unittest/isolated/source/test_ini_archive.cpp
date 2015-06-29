/*
 * test_ini_archive.cpp
 *
 *  Created on: Feb 12, 2015
 *      Author: kristone
 */

#include <gtest/gtest.h>


#include "sf/archive/ini.h"

#include <sstream>
#include <locale>

#include "../include/test_vo.h"



namespace casual
{

   namespace
   {
      namespace local
      {
         template<typename T>
         void value_to_string( const T& value, std::string& string)
         {
            sf::archive::ini::Save save;
            sf::archive::ini::Writer writer( save.target());
            writer << CASUAL_MAKE_NVP( value);
            save.serialize( string);
         }

         template<typename T>
         void string_to_value( const std::string& string, T& value)
         {
            sf::archive::ini::Load load;
            sf::archive::ini::Reader reader( load.serialize( string));
            reader >> CASUAL_MAKE_NVP( value);
         }

      } // local
   } //



   TEST( casual_sf_ini_archive, write_read_string_with_new_line)
   {
      std::string ini;
      std::string source = "foo\nbar";
      local::value_to_string( source, ini);
      std::string target;
      local::string_to_value( ini, target);

      EXPECT_TRUE( source == target);
   }

   TEST( casual_sf_ini_archive, write_read_boolean)
   {
      std::string ini;
      bool source = true;
      local::value_to_string( source, ini);
      bool target = false;
      local::string_to_value( ini, target);
      EXPECT_TRUE( source == target);
   }

   TEST( casual_sf_ini_archive, write_read_decimal)
   {
      std::string ini;
      float source = 3.14;
      local::value_to_string( source, ini);
      float target = 0.0;
      local::string_to_value( ini, target);
      EXPECT_TRUE( source == target);
   }

   TEST( casual_sf_ini_archive, write_read_container)
   {
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

            template<typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( integer);
               archive & CASUAL_MAKE_NVP( string);
            }
         };


         struct OtherVO
         {
            struct InnerVO
            {
               long long huge;

               template<typename A>
               void serialize( A& archive)
               {
                  archive & CASUAL_MAKE_NVP( huge);
               }

            };

            InnerVO first_inner;
            InnerVO other_inner;


            template<typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( first_inner);
               archive & CASUAL_MAKE_NVP( other_inner);
            }
         };

         bool boolean;
         FirstVO first;
         short tiny;
         std::vector<OtherVO> others;

         template<typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( boolean);
            archive & CASUAL_MAKE_NVP( first);
            archive & CASUAL_MAKE_NVP( tiny);
            archive & CASUAL_MAKE_NVP( others);
         }

      };

      TEST( casual_sf_ini_archive, write_read_serializable)
      {
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


      TEST( casual_sf_ini_archive, test_control_characters)
      {
         for( short idx = 0; idx < 255; ++idx)
         {
            if( std::iscntrl( static_cast<char>( idx), std::locale( "")))
            {
               std::cout << idx << std::endl;
            }
         }
      }


   }

/*
   struct TinyVO
   {
      bool boolean = true;
      long integer = 123;

      template<typename A>
      void serialize( A& archive)
      {
         archive & CASUAL_MAKE_NVP( boolean);
         archive & CASUAL_MAKE_NVP( integer);
      }
   };

   TEST( casual_sf_ini_archive, nested_stuff)
   {
      std::string ini;
      //std::vector<std::vector<std::vector<long>>> source{ {{1, 3, 5}, {2, 4, 6, 8}}, {{4, 3, 2}} };
      //std::map<long,TinyVO> source{ { 1, TinyVO()}, { 2, TinyVO()} };
      std::map<long,TinyVO> source{ { 1, TinyVO()}, { 2, TinyVO()}, { 3, TinyVO()}};
      //std::map<long,std::string> source{ { 1, "foo"}};
      local::value_to_string( source, ini);
      std::cout << ini << std::endl;
   }
*/

} // casual
