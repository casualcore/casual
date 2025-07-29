//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"


#include "common/serialize/create.h"

#include "common/buffer/type.h"

#include "common/code/casual.h"

#include <optional>

namespace casual
{
   namespace common
   {
      struct Directive
      {
         enum class Type 
         {
            relaxed,
            strict,
            consumed
         };
         Type type;
         std::string format;

         friend std::ostream& operator << ( std::ostream& out, Type value)
         {
            switch( value)
            {
               case Type::relaxed: return out << "relaxed";
               case Type::strict: return out << "strict";
               case Type::consumed: return out << "consumed";
            }
            return out << "unknown";
         }
         friend std::ostream& operator << ( std::ostream& out, const Directive& value)
         {
            return out << "{ type: " << value.type << ", format: " << value.format << '}';
         }
      };
      struct archive_create : public ::testing::TestWithParam< Directive> 
      {
         auto param() const { return ::testing::TestWithParam< Directive>::GetParam();}
      };

      namespace local
      {
         namespace
         {
            namespace create
            {
               template< typename S>
               auto reader( const Directive& directive, S&& source)
               {
                  switch( directive.type)
                  {
                     case Directive::Type::relaxed: return serialize::create::reader::relaxed::from( directive.format, source);
                     case Directive::Type::strict: return serialize::create::reader::strict::from( directive.format, source);
                     case Directive::Type::consumed: return serialize::create::reader::consumed::from( directive.format, source);
                  }
                  return serialize::create::reader::strict::from( directive.format, source);
               }
            } // create
         } // <unnamed>
      } // local

      INSTANTIATE_TEST_SUITE_P( common_serialize_create,
            archive_create,
            ::testing::Values( 
               Directive{ Directive::Type::relaxed, std::string( "yaml")},
               Directive{ Directive::Type::strict, std::string( "yaml")},
               Directive{ Directive::Type::consumed, std::string( "yaml")},
               Directive{ Directive::Type::relaxed, std::string( "json")},
               Directive{ Directive::Type::strict, std::string( "json")},
               Directive{ Directive::Type::consumed, std::string( "json")},
               Directive{ Directive::Type::relaxed, std::string( "xml")},
               Directive{ Directive::Type::strict, std::string( "xml")},
               Directive{ Directive::Type::consumed, std::string( "xml")},
               Directive{ Directive::Type::relaxed, std::string( "ini")},
               Directive{ Directive::Type::strict, std::string( "ini")},
               Directive{ Directive::Type::consumed, std::string( "ini")},
               Directive{ Directive::Type::relaxed, std::string( common::buffer::type::json)},
               Directive{ Directive::Type::strict, std::string( common::buffer::type::json)},
               Directive{ Directive::Type::consumed, std::string( common::buffer::type::json)},
               Directive{ Directive::Type::relaxed, std::string( common::buffer::type::yaml)},
               Directive{ Directive::Type::strict, std::string( common::buffer::type::yaml)},
               Directive{ Directive::Type::consumed, std::string( common::buffer::type::yaml)},
               Directive{ Directive::Type::relaxed, std::string( common::buffer::type::xml)},
               Directive{ Directive::Type::strict, std::string( common::buffer::type::xml)},
               Directive{ Directive::Type::consumed, std::string( common::buffer::type::xml)}
            ));


      TEST_P( archive_create, stream_int_value)
      {
         common::unittest::Trace trace;

         auto directive = this->param();

         auto writer = serialize::create::writer::from( directive.format);

         {
            int int_value = 42;
            writer << CASUAL_NAMED_VALUE( int_value);
         }

         {
            auto stream = writer.consume< std::stringstream>();
            auto archive = local::create::reader( directive, stream);
            int int_value = 0;
            archive >> CASUAL_NAMED_VALUE( int_value);

            EXPECT_TRUE( int_value == 42);
         }
      }


      TEST_P( archive_create, binary_int_value)
      {
         common::unittest::Trace trace;

         auto directive = this->param();

         auto writer = serialize::create::writer::from( directive.format);

         {
            int int_value = 42;
            writer << CASUAL_NAMED_VALUE( int_value);
         }

         {
            auto data = writer.consume< platform::binary::type>();
            auto archive = local::create::reader( directive, data);
            int int_value = 0;
            archive >> CASUAL_NAMED_VALUE( int_value);

            EXPECT_TRUE( int_value == 42);
         }
      }


      TEST( archive_create, create_writer_from_unknown_archive__expecting_exception)
      {
         common::unittest::Trace trace;

         EXPECT_CODE(
         {
            serialize::create::writer::from( "foo");
         }, code::casual::invalid_argument);

      }

      TEST( archive_create, create_consumed_reader_from_unknown_archive__expecting_exception)
      {
         common::unittest::Trace trace;

         EXPECT_CODE(
         {
            serialize::create::reader::consumed::from( "foo", std::cin);
         }, code::casual::invalid_argument);
      }

      TEST( archive_create, create_strict_reader_from_unknown_archive__expecting_exception)
      {
         common::unittest::Trace trace;

         EXPECT_CODE(
         {
            serialize::create::reader::strict::from( "foo", std::cin);
         }, code::casual::invalid_argument);
      }

      TEST( archive_create, create_relaxed_reader_from_unknown_archive__expecting_exception)
      {
         common::unittest::Trace trace;

         EXPECT_CODE(
         {
            serialize::create::reader::relaxed::from( "foo", std::cin);
         }, code::casual::invalid_argument);
      }

      // Test for strict reader handling of optional values
      namespace local
      {
         namespace vo
         {
            struct WithOptional
            {
               int required_field = 0;
               std::optional<int> optional_field;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE_NAME(required_field, "required_field");
                  CASUAL_SERIALIZE_NAME(optional_field, "optional_field");
               )
            };
         } // vo
      } // local

      TEST( archive_create, strict_reader_optional_missing_field_should_not_fail)
      {
         common::unittest::Trace trace;

         // Create a document with only the required field (missing optional field)
         std::string yaml_content = "required_field: 42\n";
         std::stringstream stream(yaml_content);
         
         // This should NOT fail for missing optional field in strict mode
         auto reader = serialize::create::reader::strict::from("yaml", stream);
         
         local::vo::WithOptional test_obj;
         EXPECT_NO_THROW(reader >> test_obj);
         
         EXPECT_EQ(42, test_obj.required_field);
         EXPECT_FALSE(test_obj.optional_field.has_value());
      }

      TEST( archive_create, strict_reader_optional_present_field_should_work)
      {
         common::unittest::Trace trace;

         // Create a document with both required and optional fields
         std::string yaml_content = "required_field: 42\noptional_field: 99\n";
         std::stringstream stream(yaml_content);
         
         auto reader = serialize::create::reader::strict::from("yaml", stream);
         
         local::vo::WithOptional test_obj;
         EXPECT_NO_THROW(reader >> test_obj);
         
         EXPECT_EQ(42, test_obj.required_field);
         EXPECT_TRUE(test_obj.optional_field.has_value());
         EXPECT_EQ(99, *test_obj.optional_field);
      }

      TEST( archive_create, strict_reader_optional_missing_required_field_should_fail)
      {
         common::unittest::Trace trace;

         // Create a document with only optional field (missing required field)
         std::string yaml_content = "optional_field: 99\n";
         std::stringstream stream(yaml_content);
         
         auto reader = serialize::create::reader::strict::from("yaml", stream);
         
         local::vo::WithOptional test_obj;
         // This should still fail for missing required field in strict mode
         EXPECT_CODE({ reader >> test_obj; }, code::casual::invalid_node);
      }
   } // common
} // casual
