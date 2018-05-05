//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <gtest/gtest.h>

#include "serviceframework/archive/log.h"
#include "serviceframework/log.h"


#include <sstream>

namespace casual
{
   TEST( casual_sf_archive_line, serialize_pod)
   {
      std::ostringstream out;
   }

   namespace local
   {
      namespace
      {
         struct Composite
         {
            long a = 42;
            std::string b = "foo";
            char c = 'X';
            bool d = false;

            struct Nested
            {
               long a = 42;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  archive << CASUAL_MAKE_NVP( a);
               )
            };

            std::vector< Nested> nested_vector = { { 4}, { 3}};

            CASUAL_CONST_CORRECT_SERIALIZE(
               archive << CASUAL_MAKE_NVP( a);
               archive << CASUAL_MAKE_NVP( b);
               archive << CASUAL_MAKE_NVP( c);
               archive << CASUAL_MAKE_NVP( d);
               archive << CASUAL_MAKE_NVP( nested_vector);
            )

         };
      } // <unnamed>
   } // local

   TEST( casual_sf_archive_line, has_formatter)
   {
      EXPECT_TRUE( common::log::has_formatter< local::Composite>::value);
   }

   TEST( casual_sf_archive_line, serialize_composite)
   {
      std::ostringstream out;

      local::Composite value;

      constexpr auto result = R"(value: { a: 42, b: "foo", c: X, d: false, nested_vector: [ { a: 4}, { a: 3}]})";
      common::log::write( out, "value: " , value);
      EXPECT_TRUE( out.str() == result) << out.str() << '\n' << result;
   }

} // casual