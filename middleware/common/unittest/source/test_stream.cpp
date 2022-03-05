//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>

#include "common/stream.h"

#include "common/chronology.h"
#include "common/strong/id.h"

namespace casual
{

   namespace common
   {
      TEST( common_stream, resource_id)
      {
         unittest::Trace trace;
         auto id = strong::resource::id{ -1};

         std::ostringstream out;
         stream::write( out, id);
         EXPECT_TRUE( out.str() == "E-1") << "out.str(): " << out.str();

      }

      TEST( common_stream, duration_ns)
      {
         unittest::Trace trace;

         std::ostringstream out;
         stream::write( out, std::chrono::nanoseconds( 560));
         EXPECT_TRUE( out.str() == "560ns") << "out.str(): " << out.str();

      }

      namespace local
      {
         namespace
         {
            namespace description
            {
               enum struct Enum
               {
                  a,
                  b,
                  c
               };
               constexpr std::string_view description( Enum value) noexcept
               {
                  switch( value)
                  {
                     case Enum::a: return "a";
                     case Enum::b: return "b";
                     case Enum::c: return "c";
                  }
                  return "<unknown>";
               }
            } // description

            namespace ostream
            {
               enum struct Enum
               {
                  a,
                  b,
                  c
               };
               std::ostream& operator << ( std::ostream& out, Enum value) noexcept
               {
                  switch( value)
                  {
                     case Enum::a: return out << "a";
                     case Enum::b: return out << "b";
                     case Enum::c: return out << "c";
                  }
                  return out << "<unknown>";
               }
            } // ostream

            struct Structure
            {
               struct
               {
                  description::Enum value = description::Enum::b;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( value);
                  )

               } inner;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( inner);
               )
            };
      

         } // <unnamed>
      } // local

      TEST( common_stream, enum_with_description)
      {
         unittest::Trace trace;

         EXPECT_TRUE( local::description::description( local::description::Enum::a) == "a");

         EXPECT_TRUE( string::compose( local::description::Enum::a) == "a") << string::compose( local::description::Enum::a);
         EXPECT_TRUE( string::compose( local::description::Enum::b) == "b");
         EXPECT_TRUE( string::compose( local::description::Enum::c) == "c");
      }


      TEST( common_stream, enum_with_ostream)
      {
         unittest::Trace trace;

         EXPECT_TRUE( string::compose( local::ostream::Enum::a) == "a") << string::compose( local::ostream::Enum::a);
         EXPECT_TRUE( string::compose( local::ostream::Enum::b) == "b");
         EXPECT_TRUE( string::compose( local::ostream::Enum::c) == "c");
      }

      TEST( common_stream, enum_within_structure)
      {
         unittest::Trace trace;

         EXPECT_TRUE( string::compose( local::Structure{}) == "{ inner: { value: b}}");
      }

      TEST( common_stream, message_type)
      {
         unittest::Trace trace;

         EXPECT_TRUE( string::compose( message::Type::domain_configuration_request) == "domain_configuration_request");
      }

      TEST( common_stream, error_code_enum)
      {
         unittest::Trace trace;

         EXPECT_TRUE( string::compose( code::casual::invalid_argument) == "[casual:invalid_argument]") << string::compose( code::casual::invalid_argument);
      }

   } // common
   
} // casual