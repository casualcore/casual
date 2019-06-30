//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/serialize/line.h"
#include "common/serialize/macro.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace local
         {
            namespace
            {
               struct A 
               {
                  long long_value{};
                  short short_value{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( long_value);
                     CASUAL_SERIALIZE( short_value);
                  })
               };

               struct B
               {
                  long long_value{};
                  short short_value{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( long_value);
                     CASUAL_SERIALIZE( short_value);
                  })

                  friend std::ostream& operator << ( std::ostream& out, const B& value) { return out << "overridden";}
               };
            } // <unnamed>
         } // local

         TEST( casual_serialize_line, serialize_composite)
         {
            common::unittest::Trace trace;

            local::A value{ 42, 2};

            std::ostringstream out;
            out <<  CASUAL_NAMED_VALUE( value);

            EXPECT_TRUE( out.str() == "value: { long_value: 42, short_value: 2}") << CASUAL_NAMED_VALUE( value);
         }

         TEST( casual_serialize_line, serialize_overridden_ostream_stream_operator)
         {
            common::unittest::Trace trace;

            local::B value{ 42, 2};

            std::ostringstream out;
            out << "value: " << value;

            EXPECT_TRUE( out.str() == "value: overridden") << CASUAL_NAMED_VALUE( value);
         }
      } //serialize
   } // common
} // casual