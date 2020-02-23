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
      std::ostringstream out;
      {
         auto archive = common::serialize::log::writer( out);
         long value = 10;
         archive << CASUAL_NAMED_VALUE( value);
      }
      EXPECT_TRUE( out.str() == "value: 10\n") << "out.str(): " << out.str();
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
      std::ostringstream out;
      {
         auto archive = common::serialize::log::writer( out);
         local::Composite composite;
         archive << CASUAL_NAMED_VALUE( composite);
      }

      constexpr auto expected = R"(composite: {
  a: 42
  b: "foo"
  c: 'X'
}
)";


      EXPECT_TRUE( out.str() == expected) << "out.str(): '" << out.str() << "'expected: '" << expected << "'"; 
   }

} // casual