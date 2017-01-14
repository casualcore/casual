//!
//! casual
//!

#include <gtest/gtest.h>


#include "sf/archive/maker.h"
#include "sf/exception.h"

#include "common/mockup/file.h"

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



   TEST( casual_sf_maker, make_reader_from_unknown_file__expecting_exception)
   {
      EXPECT_THROW(
      {
         sf::archive::reader::from::file( common::file::name::unique( "does_not_exists", ".ini"));
      }, sf::exception::invalid::File);
   }

   TEST( casual_sf_maker, make_writer_from_unknown_archive__expecting_exception)
   {
      EXPECT_THROW(
      {
         sf::archive::writer::from::file( "casual.foo");
      }, sf::exception::invalid::File);
   }



   TEST( casual_sf_maker, write_and_read_some__expecting_equality)
   {

      auto file = common::mockup::file::temporary::name( ".ini");

      long source = 42;
      {
         Banana banana;
         banana.integer = source;

         auto w = sf::archive::writer::from::file( file);

         w << CASUAL_MAKE_NVP( banana);
      }

      long target;
      {
         Banana banana;
         auto r = sf::archive::reader::from::file( file);

         r >> CASUAL_MAKE_NVP( banana);

         target = banana.integer;
      }

      EXPECT_TRUE( source == target);

   }

} // casual


