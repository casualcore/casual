
#include "common/unittest.h"

#include "common/value/id.h"


namespace casual
{
   namespace common
   {
      
      TEST( casual_common_value_id, value_by_value)
      {
         common::unittest::Trace trace;

         value::basic_id< int> id;
         EXPECT_TRUE( ( std::is_same< int, decltype( id.value())>::value));
         EXPECT_TRUE( ( ! std::is_lvalue_reference< decltype( id.value())>::value));
      }

      TEST( casual_common_value_id, value_const_ref)
      {
         common::unittest::Trace trace;

         value::basic_id< std::string> id;
         EXPECT_TRUE( ( std::is_same< const std::string&, decltype( id.value())>::value));
         EXPECT_TRUE( ( std::is_lvalue_reference< decltype( id.value())>::value));
      }


      TEST( casual_common_value_id, custom_initializer)
      {
         common::unittest::Trace trace;

         struct default_initialize_42
         {
            constexpr static auto initialize() { return 42;}
         };


         {
            value::basic_id< int, default_initialize_42> id;
            EXPECT_TRUE( id.value() == 42);
         }

         {
            value::basic_id< int, default_initialize_42> id{ 1} ;
            EXPECT_TRUE( id.value() == 1);
         }
      }

      TEST( casual_common_value_id, compare)
      {
         common::unittest::Trace trace;

         value::basic_id< int> id_1( 1);
         value::basic_id< int> id_2( 2);
         value::basic_id< int> id_3( 3);

         EXPECT_TRUE( id_1 == id_1);
         EXPECT_TRUE( id_1 <= id_1);
         EXPECT_TRUE( id_1 >= id_1);

         EXPECT_TRUE( id_1 < id_2);
         EXPECT_TRUE( id_2 < id_3);
         EXPECT_TRUE( id_1 < id_3);
         EXPECT_TRUE( id_1 <= id_3);

         EXPECT_TRUE( id_3 > id_1);
         EXPECT_TRUE( id_3 >= id_1);
      }

   } // common
} // casual
