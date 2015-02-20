/*
 * test_ini_archive.cpp
 *
 *  Created on: Feb 12, 2015
 *      Author: kristone
 */

#include <gtest/gtest.h>


#include "sf/archive/ini.h"

#include <sstream>

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

   namespace
   {
      struct CompositeVO
      {
         struct SomeFirstVO
         {
            bool first_boolean = true;
            long first_integer = 123456;

            std::vector< std::vector<short>> first_nested{ { 1, 2, 3}, { 4, 5, 6}};
            std::vector< char> first_binary{ 'a', 'b', 'c'};

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( first_boolean);
               archive & CASUAL_MAKE_NVP( first_integer);
               //archive & CASUAL_MAKE_NVP( first_nested);
               //archive & CASUAL_MAKE_NVP( first_binary);
            }
         };

         struct SomeOtherVO
         {
            std::vector<long> other_large_ones{ 777, 888, 999};
            std::string other_string = "some text";
            char other_character = 'c';

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( other_large_ones);
               archive & CASUAL_MAKE_NVP( other_string);
               archive & CASUAL_MAKE_NVP( other_character);
            }
         };

         struct SomeThirdVO
         {
            std::vector<long> third_large_ones{ 7, 8, 9};
            double third_decimal = 123.456;
            std::vector<short> third_small_ones{ 1, 2, 3};

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( third_large_ones);
               archive & CASUAL_MAKE_NVP( third_decimal);
               archive & CASUAL_MAKE_NVP( third_small_ones);
            }
         };



         SomeFirstVO composite_first;
         bool composite_boolean = false;
         std::vector<SomeOtherVO> composite_other{ SomeOtherVO(), SomeOtherVO()};
         std::string composite_string = "ok?";
         SomeThirdVO composite_third;
         std::vector<long> composite_integers{ 1, 2, 3};

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( composite_first);
            archive & CASUAL_MAKE_NVP( composite_boolean);
            archive & CASUAL_MAKE_NVP( composite_other);
            archive & CASUAL_MAKE_NVP( composite_string);
            archive & CASUAL_MAKE_NVP( composite_third);
            archive & CASUAL_MAKE_NVP( composite_integers);

         }

      };
   }


   TEST( casual_sf_ini_archive, test_some)
   {
/*
      std::string ini;

      {
         CompositeVO value;

         local::value_to_string( value, ini);

         std::cout << ini << std::endl;
      }

      std::cout << std::endl;

      {
         CompositeVO value;

         local::string_to_value( ini, value);

         value.composite_integers.clear();
         value.composite_string = "no original";

         local::value_to_string( value, ini);

         std::cout << ini << std::endl;
      }

      std::cout << std::endl;

*/
      {
         std::string ini;
         std::vector<std::vector<std::vector<long>>> source{ {{1, 3, 5}, {2, 4, 6}}, {{5, 3, 1}, {6, 4, 2}} };
         local::value_to_string( source, ini);
         std::cout << ini << std::endl;

         std::vector<std::vector<std::vector<long>>> target;
         local::string_to_value( ini, target);

         std::cout << std::boolalpha << (source == target) << std::noboolalpha << std::endl;

         local::value_to_string( target, ini);
         std::cout << ini << std::endl;

         //local::string_to_value( ini, integers);
         //local::value_to_string( integers, ini);
         //std::cout << ini << std::endl;
      }

   }

} // casual
