
#include "common/unittest.h"

#include "common/optional.h"


namespace casual
{
   namespace common
   {
      
      TEST( casual_common_optional, default_ctor)
      {
         common::unittest::Trace trace;

         value::optional< int, -1> value;

         EXPECT_TRUE( value.empty());
      }


      TEST( casual_common_optional, in_hash_map)
      {
         common::unittest::Trace trace;

         value::optional< int, -1> value;

         std::unordered_map< value::optional< int, -1>, int> map{
            {
               value, 1
            }
         };


         EXPECT_TRUE( map.at( value) == 1);
      }
   } // common
} // casual