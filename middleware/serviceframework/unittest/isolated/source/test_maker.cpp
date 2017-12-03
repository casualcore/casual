//!
//! casual
//!

#include "common/unittest.h"


#include "sf/archive/maker.h"

#include "sf/exception.h"
#include "common/exception/system.h"

#include <sstream>

namespace casual
{

   namespace
   {

      struct Banana
      {
         long integer = 0;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            archive & CASUAL_MAKE_NVP( integer);
         )
      };
   } // <unnamed>


/*
   TEST( casual_sf_maker, sandbox)
   {
      Banana banana;
      banana.integer = 42;

      auto w = sf::archive::writer::from::name( "ini");

      w << CASUAL_MAKE_NVP( banana);
   }
*/

   TEST( casual_sf_maker, decuce_archive_from_yml_input__expecting_success)
   {
      common::unittest::Trace trace;

      Banana banana;
      banana.integer = 42;

      std::stringstream stream;

      {
         auto w = sf::archive::writer::from::name( stream, "yml");
         w << CASUAL_MAKE_NVP( banana);
      }


      EXPECT_NO_THROW
      ({
         sf::archive::reader::from::data( stream);
      });
   }


   TEST( casual_sf_maker, decuce_archive_from_xml_input__expecting_success)
   {
      common::unittest::Trace trace;

      Banana banana;
      banana.integer = 42;

      std::stringstream stream;

      {
         auto w = sf::archive::writer::from::name( stream, "xml");
         w << CASUAL_MAKE_NVP( banana);
      }

      EXPECT_TRUE( ! stream.str().empty());

      EXPECT_NO_THROW
      ({
         sf::archive::reader::from::data( stream);
      }) << "stream: " << stream.str();
   }

   TEST( casual_sf_maker, decuce_archive_from_jsn_input__expecting_success)
   {
      common::unittest::Trace trace;

      Banana banana;
      banana.integer = 42;

      std::stringstream stream;

      {
         auto w = sf::archive::writer::from::name( stream, "jsn");
         w << CASUAL_MAKE_NVP( banana);
      }

      EXPECT_NO_THROW
      ({
         sf::archive::reader::from::data( stream);
      });
   }

   TEST( casual_sf_maker, decuce_archive_from_ini_input__expecting_success)
   {
      common::unittest::Trace trace;

      Banana banana;
      banana.integer = 42;

      std::stringstream stream;

      {
         auto w = sf::archive::writer::from::name( stream, "ini");
         w << CASUAL_MAKE_NVP( banana);
      }

      EXPECT_NO_THROW
      ({
         sf::archive::reader::from::data( stream);
      });
   }



   TEST( casual_sf_maker, read_from_non_existing_file__expecting_exception)
   {
      EXPECT_THROW
      ({
         sf::archive::reader::from::file( common::file::name::unique( common::directory::temporary() + "/", ".ini"));
      },common::exception::system::invalid::File);
   }

   TEST( casual_sf_maker, write_and_read_file__expecting_success)
   {
      common::file::scoped::Path path{ common::file::name::unique( common::directory::temporary() + "/", ".ini")};

      long source = 42;
      {
         Banana banana;
         banana.integer = source;

         auto w = sf::archive::writer::from::file( path);

         w << CASUAL_MAKE_NVP( banana);
      }

      long target;
      {
         Banana banana;
         auto r = sf::archive::reader::from::file( path);

         r >> CASUAL_MAKE_NVP( banana);

         target = banana.integer;
      }

      EXPECT_TRUE( source == target);
   }

   TEST( casual_sf_maker, write_and_read_buffer__expecting_success)
   {
      common::unittest::Trace trace;

      sf::platform::binary::type buffer;

      long source = 42;
      {
         Banana banana;
         banana.integer = source;

         auto w = sf::archive::writer::from::buffer( buffer, "json");
         w << CASUAL_MAKE_NVP( banana);
      }

      long target = 0;
      {
         Banana banana;
         auto r = sf::archive::reader::from::buffer( buffer, "json");
         r >> CASUAL_MAKE_NVP( banana);

         target = banana.integer;
      }

      EXPECT_TRUE( source == target);
   }

   TEST( casual_sf_maker, make_writer_from_unknown_archive__expecting_exception)
   {
      common::unittest::Trace trace;

      EXPECT_THROW(
      {
         sf::archive::writer::from::name( std::cout, "foo");
      }, sf::exception::Validation);

   }

   TEST( casual_sf_maker, make_writer_with_capital_letters__expecting_success)
   {
      common::unittest::Trace trace;

      EXPECT_NO_THROW(
      {
         sf::archive::writer::from::name( "XML");
      });

   }


   TEST( casual_sf_maker, write_and_read_some__expecting_equality)
   {
      common::unittest::Trace trace;

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

