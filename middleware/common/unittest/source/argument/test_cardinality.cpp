//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "casual/argument/cardinality.h"

namespace casual
{
   using namespace common;

   static_assert( argument::cardinality::one().valid( 1));
   static_assert( argument::cardinality::one_many().valid( 3));
   static_assert( argument::cardinality::min( 4).valid( 4));
   static_assert( ! argument::cardinality::min( 4).valid( 3));

   static_assert( argument::cardinality::zero_one() + argument::cardinality::one_many() == argument::cardinality::one_many());
   static_assert( argument::cardinality::fixed( 4) + argument::cardinality::one_many() == argument::cardinality::min( 5));
   static_assert( argument::cardinality::one_many() + argument::cardinality::fixed( 4) == argument::cardinality::min( 5));
   static_assert( argument::cardinality::fixed( 20) + argument::cardinality::fixed( 22) == argument::cardinality::fixed( 42));

   TEST( argument_cardinality, invalid)
   {
      unittest::Trace trace;

      ASSERT_CODE( ( argument::Cardinality{ 2, 1}), code::casual::invalid_argument);
      ASSERT_CODE( ( argument::Cardinality{ -1, 2}), code::casual::invalid_argument);
      ASSERT_CODE( ( argument::Cardinality{ -2, -1}), code::casual::invalid_argument);
   }

   TEST( argument_cardinality, equality)
   {
      unittest::Trace trace;

      static_assert( argument::cardinality::one_many() == argument::cardinality::min( 1));
      EXPECT_TRUE( argument::cardinality::one_many() == argument::cardinality::min( 1));
   }
   
} // casual