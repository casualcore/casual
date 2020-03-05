//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <gtest/gtest.h>

#include "common/serialize/log.h"
#include "common/serialize/macro.h"


#include <sstream>

namespace casual
{
   TEST( common_serialize_log_writer, serialize_pod)
   {
      auto archive = common::serialize::log::writer();
      {
         
         long value = 10;
         archive << CASUAL_NAMED_VALUE( value);
      }
      auto result = archive.consume< std::string>();
      EXPECT_TRUE( result == "value: 10\n") << "out.str(): " << result;
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

            CASUAL_CONST_CORRECT_SERIALIZE(
               archive << CASUAL_NAMED_VALUE( a);
               archive << CASUAL_NAMED_VALUE( b);
               archive << CASUAL_NAMED_VALUE( c);
            )

         };
      } // <unnamed>
   } // local

   TEST( common_serialize_log_writer, serialize_composite)
   {
      auto archive = common::serialize::log::writer();
      {
         local::Composite composite;
         archive << CASUAL_NAMED_VALUE( composite);
      }

      constexpr auto expected = R"(composite: {
  a: 42
  b: "foo"
  c: 'X'
}
)";

      auto result = archive.consume< std::string>();
      EXPECT_TRUE( result == expected) << "result: '" << result << "'expected: '" << expected << "'"; 
   }

} // casual