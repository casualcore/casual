//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/unittest.h"
#include "common/predicate.h"
#include "common/algorithm/is.h"
#include "common/array.h"


namespace casual
{

   namespace common
   {
      TEST( casual_common_predicate, negate)
      {
         common::unittest::Trace trace;

         auto pred = predicate::negate(
            []( int value){ return value == 1;}
         ); 

         EXPECT_TRUE( pred( 0));
         EXPECT_FALSE( pred( 1));
         EXPECT_TRUE( pred( 2));
      }

      TEST( casual_common_predicate_conjunction, one_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::conjunction(
            []( int l, int r){ return l < r;}
         ); 

         EXPECT_TRUE( pred( 1, 2));
         EXPECT_FALSE( pred( 1, 1));
         EXPECT_FALSE( pred( 2, 1));
      }

      TEST( casual_common_predicate_conjunction, two_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::conjunction(
            []( int l, int r){ return l < r;},
            []( int l, int r){ return ( l + r) % 2 == 0;}
         ); 

         EXPECT_TRUE( pred( 2, 4));
         EXPECT_FALSE( pred( 1, 2));
         EXPECT_FALSE( pred( 2, 2));
      }

      TEST( casual_common_predicate_disjunction, one_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::disjunction(
            []( int l, int r){ return l < r;}
         ); 

         EXPECT_TRUE( pred( 1, 2));
         EXPECT_FALSE( pred( 1, 1));
         EXPECT_FALSE( pred( 2, 1));
      }

      TEST( casual_common_predicate_disjunction, two_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::disjunction(
            []( int l, int r){ return l < r;},
            []( int l, int r){ return ( l + r) % 2 == 0;}
         ); 

         EXPECT_TRUE( pred( 2, 4));
         EXPECT_FALSE( pred( 2, 1));
         EXPECT_TRUE( pred( 2, 2));
         EXPECT_TRUE( pred( 4, 2));
      }

      TEST( casual_common_predicate_disjunction, order__one_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::disjunction(
            []( int l, int r){ return l < r;}
         ); 

         std::vector< int> values{ 4, 3, 2, 1, 4, 4, 2, 3};
         EXPECT_TRUE( ! algorithm::is::sorted( values));

         algorithm::sort( values, pred);

         EXPECT_TRUE( algorithm::is::sorted( values));
         EXPECT_TRUE( algorithm::is::sorted( values, pred));
      }

      namespace local
      {
         namespace
         {
            struct State : Compare< State>
            {
               State( int a, int b) : a{ a}, b{ b} {}
               
               int a{};
               int b{};
               
               auto tie() const noexcept { return std::tie( a, b);}
               friend std::ostream& operator << ( std::ostream& out, const State& s) { return out << "{ a: " << s.a << ", b: " << s.b << '}';}
            };

            namespace filter 
            {
               auto a( int value)
               {
                  return [ value]( auto& state){ return state.a == value;};
               }

               auto b( int value)
               {
                  return [ value]( auto& state){ return state.b == value;};
               }

               auto even()
               {
                  return []( auto value){ return value % 2 == 0;};
               }

            } // filter 


      
         } // <unnamed>
      } // local

      TEST( casual_common_predicate_conjunction, rvalue_lambdas__a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         auto pred = predicate::conjunction(
            []( const auto& s){ return s.a == 1;},
            []( const auto& s){ return s.b == 2;}
         ); 

         auto filtered = std::get< 0>( algorithm::partition( values, pred));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( algorithm::equal( filtered, expected)) << trace.compose( "filtered: ", filtered);
      }

      TEST( casual_common_predicate_conjunction, lvalue_lambdas__a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         const auto a = []( const local::State& s){ return s.a == 1;};
         const auto b = []( const local::State& s){ return s.b == 2;};
         
         auto pred = predicate::conjunction( a, b); 

         auto filtered = std::get< 0>( algorithm::partition( values, pred));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( algorithm::equal( filtered, expected)) << trace.compose( "filtered: ", filtered);
      }

      TEST( casual_common_predicate, a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         auto filtered = std::get< 0>( algorithm::partition( values, local::filter::a( 1)));
         filtered = std::get< 0>( algorithm::partition( filtered, local::filter::b( 2)));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( algorithm::equal( filtered, expected)) << trace.compose( "filtered: ", filtered);
      }

      TEST( casual_common_predicate_conjunction, rvalue_functors__a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         auto pred = predicate::conjunction(
            local::filter::a( 1),
            local::filter::b( 2)
         ); 

         auto filtered = std::get< 0>( algorithm::partition( values, pred));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( algorithm::equal( filtered, expected)) << trace.compose( "filtered: ", filtered);
      }

      TEST( casual_common_predicate_conjunction, lvalue_functors__a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         auto a = local::filter::a( 1);
         auto b = local::filter::b( 2);

         auto pred = predicate::conjunction( a, b); 

         auto filtered = std::get< 0>( algorithm::partition( values, pred));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( algorithm::equal( filtered, expected)) << trace.compose( "filtered: ", filtered);
      }

      
      TEST( casual_common_predicate, composition__even_a)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         auto filtered = algorithm::sort( algorithm::filter( values, predicate::composition( local::filter::even(), []( auto& state){ return state.a;})));

         EXPECT_TRUE( algorithm::equal( filtered, array::make( local::State{ 2, 2}, local::State{ 4, 7}))) << CASUAL_NAMED_VALUE( filtered);
      }
   
   } // common
} // casual