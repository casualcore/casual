/*
 * test_maker.cpp
 *
 *  Created on: Apr 30, 2016
 *      Author: kristone
 */

#include <gtest/gtest.h>

#include "sf/archive/maker.h"

#include "sf/exception.h"

#include <sstream>

namespace casual
{

   namespace
   {

      struct Banana
      {
         long integer = 0;

         template<typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( integer);
         }
      };
   } // <unnamed>


   TEST( casual_sf_maker, read_from_non_existing_file__expecting_exception)
   {
      EXPECT_THROW
      ({
         sf::archive::reader::from::file( "hopefully_this_file_does_not_exist.ini");
      },sf::exception::FileNotOpen);
   }

   TEST( casual_sf_maker, write_and_read_file__expecting_success)
   {
      long source = 42;
      {
         Banana banana;
         banana.integer = source;

         auto w = sf::archive::writer::from::file( "/tmp/casual.ini");

         w << CASUAL_MAKE_NVP( banana);
      }

      long target;
      {
         Banana banana;
         auto r = sf::archive::reader::from::file( "/tmp/casual.ini");

         r >> CASUAL_MAKE_NVP( banana);

         target = banana.integer;
      }

      EXPECT_TRUE( source == target);
   }

   TEST( casual_sf_maker, make_writer_from_unknown_archive__expecting_exception)
   {

      EXPECT_THROW(
      {
         sf::archive::writer::from::name( std::cout, "foo");
      }, sf::exception::Validation);

   }

   TEST( casual_sf_maker, make_writer_with_capital_letters__expecting_success)
   {

      EXPECT_NO_THROW(
      {
         sf::archive::writer::from::name( "XML");
      });

   }


   TEST( casual_sf_maker, write_and_read_some__expecting_equality)
   {
      std::stringstream stream;

      long source = 42;
      {
         Banana banana;
         banana.integer = source;

         auto w = sf::archive::writer::from::name( stream, "ini");

         w << CASUAL_MAKE_NVP( banana);
      }

      long target;
      {
         Banana banana;
         auto r = sf::archive::reader::from::name( stream, "ini");

         r >> CASUAL_MAKE_NVP( banana);

         target = banana.integer;
      }

      EXPECT_TRUE( source == target);

   }


} // casual


