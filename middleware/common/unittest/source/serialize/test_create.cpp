//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"


#include "common/serialize/create.h"

#include "common/buffer/type.h"

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

      INSTANTIATE_TEST_CASE_P( common_serialize_create,
            archive_create,
            ::testing::Values( 
               Directive{ Directive::Type::relaxed, "yaml"},
               Directive{ Directive::Type::strict, "yaml"},
               Directive{ Directive::Type::consumed, "yaml"},
               Directive{ Directive::Type::relaxed, "json"},
               Directive{ Directive::Type::strict, "json"},
               Directive{ Directive::Type::consumed, "json"},
               Directive{ Directive::Type::relaxed, "xml"},
               Directive{ Directive::Type::strict, "xml"},
               Directive{ Directive::Type::consumed, "xml"},
               Directive{ Directive::Type::relaxed, "ini"},
               Directive{ Directive::Type::strict, "ini"},
               Directive{ Directive::Type::consumed, "ini"},
               Directive{ Directive::Type::relaxed, common::buffer::type::json()},
               Directive{ Directive::Type::strict, common::buffer::type::json()},
               Directive{ Directive::Type::consumed, common::buffer::type::json()},
               Directive{ Directive::Type::relaxed, common::buffer::type::yaml()},
               Directive{ Directive::Type::strict, common::buffer::type::yaml()},
               Directive{ Directive::Type::consumed, common::buffer::type::yaml()},
               Directive{ Directive::Type::relaxed, common::buffer::type::xml()},
               Directive{ Directive::Type::strict, common::buffer::type::xml()},
               Directive{ Directive::Type::consumed, common::buffer::type::xml()}
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

         EXPECT_THROW(
         {
            serialize::create::writer::from( "foo");
         }, exception::system::invalid::Argument);

      }

      TEST( archive_create, create_consumed_reader_from_unknown_archive__expecting_exception)
      {
         common::unittest::Trace trace;

         EXPECT_THROW(
         {
            serialize::create::reader::consumed::from( "foo", std::cin);
         }, exception::system::invalid::Argument);
      }

      TEST( archive_create, create_strict_reader_from_unknown_archive__expecting_exception)
      {
         common::unittest::Trace trace;

         EXPECT_THROW(
         {
            serialize::create::reader::strict::from( "foo", std::cin);
         }, exception::system::invalid::Argument);
      }

      TEST( archive_create, create_relaxed_reader_from_unknown_archive__expecting_exception)
      {
         common::unittest::Trace trace;

         EXPECT_THROW(
         {
            serialize::create::reader::relaxed::from( "foo", std::cin);
         }, exception::system::invalid::Argument);
      }
   } // common
} // casual
