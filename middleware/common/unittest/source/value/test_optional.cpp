
#include "common/unittest.h"

#include "common/value/optional.h"


namespace casual
{
   namespace common
   {
      
      TEST( casual_common_optional, default_ctor)
      {
         common::unittest::Trace trace;

         value::Optional< int, -1> value;

         EXPECT_TRUE( ! value);
      }

      TEST( casual_common_optional, ctor)
      {
         common::unittest::Trace trace;

         value::Optional< int, -1> value{ 42};

         EXPECT_TRUE( value);
         EXPECT_TRUE( value.value() == 42);
      }


      TEST( casual_common_optional, in_hash_map)
      {
         common::unittest::Trace trace;

         using opt = value::Optional< int, -1>;

         opt value;

         std::unordered_map< opt, int> map{
            { value, 1 },
            { opt( 42), 42 },
         };


         EXPECT_TRUE( map.at( value) == 1);
         EXPECT_TRUE( map.at( opt( 42)) == 42);
      }
   } // common
} // casual
