//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"


#include "serviceframework/archive/create.h"

#include "common/buffer/type.h"

namespace casual
{
   namespace serviceframework
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
                     case Directive::Type::relaxed: return archive::create::reader::relaxed::from( directive.format, source);
                     case Directive::Type::strict: return archive::create::reader::strict::from( directive.format, source);
                     case Directive::Type::consumed: return archive::create::reader::consumed::from( directive.format, source);
                  }
                  return archive::create::reader::strict::from( directive.format, source);
               }
            } // create
         } // <unnamed>
      } // local

      INSTANTIATE_TEST_CASE_P( serviceframework_archive_create,
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

         std::stringstream data;

         {
            auto archive = archive::create::writer::from( directive.format, data);
            int int_value = 42;

            archive << CASUAL_MAKE_NVP( int_value);
         }

         {
            auto archive = local::create::reader( directive, data);
            int int_value = 0;
            archive >> CASUAL_MAKE_NVP( int_value);

            EXPECT_TRUE( int_value == 42);
         }
      }


      TEST_P( archive_create, binary_int_value)
      {
         common::unittest::Trace trace;

         auto directive = this->param();

         platform::binary::type data;

         {
            auto archive = archive::create::writer::from( directive.format, data);
            int int_value = 42;

            archive << CASUAL_MAKE_NVP( int_value);
         }

         {
            auto archive = local::create::reader( directive, data);
            int int_value = 0;
            archive >> CASUAL_MAKE_NVP( int_value);

            EXPECT_TRUE( int_value == 42);
         }
      }


      TEST( archive_create, create_writer_from_unknown_archive__expecting_exception)
      {
         common::unittest::Trace trace;

         EXPECT_THROW(
         {
            archive::create::writer::from( "foo", std::cout);
         }, serviceframework::exception::Validation);

      }

   } // serviceframework
} // casual