//!
//! casual
//!

#include <gtest/gtest.h>


#include "sf/archive/xml.h"

#include <sstream>

#include "../../include/test_vo.h"

namespace casual
{

   namespace local
   {
      namespace
      {
         template<typename T>
         void value_to_string( const T& value, std::string& string)
         {
            sf::archive::xml::Save save;
            sf::archive::xml::Writer writer( save());
            writer << CASUAL_MAKE_NVP( value);
            save( string);
         }

         template<typename T>
         void string_to_value( const std::string& string, T& value)
         {
            sf::archive::xml::Load load;
            sf::archive::xml::Reader reader( load( string));
            reader >> CASUAL_MAKE_NVP( value);
         }

      }
   }


   TEST( casual_sf_xml_archive, relaxed_read_serializible)
   {
      sf::archive::xml::Load load;
      load( test::SimpleVO::xml());

      sf::archive::xml::relaxed::Reader reader( load());

      test::SimpleVO value;

      reader >> CASUAL_MAKE_NVP( value);

      EXPECT_TRUE( value.m_long == 234) << value.m_long;
      EXPECT_TRUE( value.m_string == "bla bla bla bla") << value.m_long;
      EXPECT_TRUE( value.m_longlong == 1234567890123456789) <<  value.m_longlong;

   }

   TEST( casual_sf_xml_archive, write_read_vector_long)
   {
      std::string xml;

      {
         std::vector< long> value{ 1, 2, 3, 4, 5, 6, 7, 8};

         local::value_to_string( value, xml);
      }

      {
         std::vector< long> value;
         local::string_to_value( xml, value);


         ASSERT_TRUE( value.size() == 8) << value.size();
         EXPECT_TRUE( value.at( 7) == 8) << value.at( 7);
      }

   }

   TEST( casual_sf_xml_archive, simple_write_read)
   {
      std::string xml;

      {
         test::SimpleVO value;
         value.m_long = 666;
         value.m_short = 42;
         value.m_string = "bla bla bla";

         local::value_to_string( value, xml);
      }

      {

         test::SimpleVO value;
         local::string_to_value( xml, value);

         EXPECT_TRUE( value.m_long == 666) << value.m_long;
         EXPECT_TRUE( value.m_short == 42) << value.m_short;
         EXPECT_TRUE( value.m_string == "bla bla bla") << value.m_string;
      }

   }


   TEST( casual_sf_xml_archive, complex_write_read)
   {
      std::string xml;

      {
         test::Composite composite;
         composite.m_values.resize( 3);
         std::map< long, test::Composite> value { { 1, composite}, { 2, composite}};

         local::value_to_string( value, xml);
      }

      {
         std::map< long, test::Composite> value;
         local::string_to_value( xml, value);

         ASSERT_TRUE( value.size() == 2);

         EXPECT_TRUE( value.at( 1).m_values.front().m_string == "foo");

      }
   }

   TEST( casual_sf_xml_archive, write_and_read_negative_long_and_empty_string__expecting_equality)
   {
      std::string xml;

      {
         test::SimpleVO value;
         value.m_long = -123;
         value.m_string = "";

         local::value_to_string( value, xml);
      }

      {

         test::SimpleVO value;
         local::string_to_value( xml, value);

         EXPECT_TRUE( value.m_long == -123) << value.m_long;
         EXPECT_TRUE( value.m_string.empty()) << value.m_string;
      }

   }

   TEST( casual_sf_xml_archive, load_invalid_document__expecting_exception)
   {
      const std::string xml{ "<?xml version='1.0'?><root>" };

      EXPECT_THROW
      ({
         sf::archive::xml::Load{}( xml);
      }, sf::exception::archive::invalid::Document);

   }

   TEST( casual_sf_xml_archive, read_with_invalid_long__expecting_exception)
   {
      const std::string xml
      {
         R"(<?xml version="1.0"?>
<value>
   <m_bool>false</m_bool>
   <m_long>1234 foo</m_long>
   <m_string>bla bla bla bla</m_string>
   <m_short>23</m_short>
   <m_longlong>1234567890123456789</m_longlong>
   <m_time>1234567890</m_time>
</value>
)"
      };

      EXPECT_THROW
      ({
         test::SimpleVO value;
         local::string_to_value( xml, value);
      }, sf::exception::archive::invalid::Node);

   }

   TEST( casual_sf_xml_archive, read_with_invalid_bool__expecting_exception)
   {
      const std::string xml
      {
         R"(<?xml version="1.0"?>
<value>
   <m_bool>nein</m_bool>
   <m_long>234</m_long>
   <m_string>bla bla bla bla</m_string>
   <m_short>23</m_short>
   <m_longlong>1234567890123456789</m_longlong>
   <m_time>1234567890</m_time>
</value>
)"
      };

      EXPECT_THROW
      ({
         test::SimpleVO value;
         local::string_to_value( xml, value);
      }, sf::exception::archive::invalid::Node);
   }

   TEST( casual_sf_xml_archive, read_with_too_long_short__expecting_exception)
   {
      const std::string xml
      {
         R"(<?xml version="1.0"?>
<value>
   <m_bool>false</m_bool>
   <m_long>234</m_long>
   <m_string>bla bla bla bla</m_string>
   <m_short>1234567890123456789</m_short>
   <m_longlong>1234567890123456789</m_longlong>
   <m_time>1234567890</m_time>
</value>
)"
      };

      EXPECT_THROW
      ({
         test::SimpleVO value;
         local::string_to_value( xml, value);
      }, sf::exception::archive::invalid::Node);

   }



} // casual

