//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <gtest/gtest.h>

#include "serviceframework/archive/log.h"


#include <sstream>

namespace casual
{
   TEST( casual_sf_log_writer, serialize_pod)
   {
      std::ostringstream out;
      {
         auto archive = serviceframework::archive::log::writer( out);
         long value = 10;
         archive << CASUAL_MAKE_NVP( value);
      }
      EXPECT_TRUE( out.str() == "-value..[10]\n") << "out.str(): " << out.str();
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
               archive << CASUAL_MAKE_NVP( a);
               archive << CASUAL_MAKE_NVP( b);
               archive << CASUAL_MAKE_NVP( c);
            )

         };
      } // <unnamed>
   } // local

   TEST( casual_sf_log_writer, serialize_composite)
   {
      std::ostringstream out;
      {
         auto archive = serviceframework::archive::log::writer( out);
         local::Composite composite;
         archive << CASUAL_MAKE_NVP( composite);
      }

      auto expected = R"(-composite
|-a..[42]
|-b..[foo]
|-c..[X]
)";


      EXPECT_TRUE( out.str() == expected) << "out.str(): " << out.str() << "expected: " << expected; 
   }

} // casual