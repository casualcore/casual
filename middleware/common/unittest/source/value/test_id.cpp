//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/value/id.h"


namespace casual
{
   namespace common
   {
      TEST( casual_common_value_id, unique_initialize)
      {
         common::unittest::Trace trace;

         struct tag{};
         using id_type = value::basic_id< int, value::id::policy::unique_initialize< int, tag, 1>>;

         id_type id;

         EXPECT_TRUE( id.value() == 1);
         EXPECT_TRUE( id == id_type{ 1});
      }
      
      TEST( casual_common_value_id, int_value_by_value)
      {
         common::unittest::Trace trace;

         value::basic_id< int> id;
         EXPECT_TRUE( ( std::is_same< int, decltype( id.value())>::value));
         EXPECT_TRUE( ( ! std::is_lvalue_reference< decltype( id.value())>::value));
      }

      TEST( casual_common_value_id, string_value_const_ref)
      {
         common::unittest::Trace trace;

         value::basic_id< std::string> id;
         EXPECT_TRUE( ( std::is_same< const std::string&, decltype( id.value())>::value));
         EXPECT_TRUE( ( std::is_lvalue_reference< decltype( id.value())>::value));
      }

      TEST( casual_common_value_id, array_max_by_value_size__expect_value__by_value)
      {
         common::unittest::Trace trace;
         using type = std::array< char, platform::size::by::value::max>;
         value::basic_id< type> id;
         EXPECT_TRUE( ( std::is_same< type, decltype( id.value())>::value));
         EXPECT_TRUE( ( ! std::is_lvalue_reference< decltype( id.value())>::value));
      }

      TEST( casual_common_value_id, array_max_by_value_size__plus_1___expect_value__const_ref)
      {
         common::unittest::Trace trace;
         using type = std::array< char, platform::size::by::value::max + 1>;
         value::basic_id< type> id;
         EXPECT_TRUE( ( std::is_same< const type&, decltype( id.value())>::value));
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
