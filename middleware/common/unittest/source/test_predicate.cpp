//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/unittest.h"
#include "common/predicate.h"
#include "common/algorithm/is.h"


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

      TEST( casual_common_predicate, and__one_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::make_and(
            []( int l, int r){ return l < r;}
         ); 

         EXPECT_TRUE( pred( 1, 2));
         EXPECT_FALSE( pred( 1, 1));
         EXPECT_FALSE( pred( 2, 1));
      }

      TEST( casual_common_predicate, and__two_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::make_and(
            []( int l, int r){ return l < r;},
            []( int l, int r){ return ( l + r) % 2 == 0;}
         ); 

         EXPECT_TRUE( pred( 2, 4));
         EXPECT_FALSE( pred( 1, 2));
         EXPECT_FALSE( pred( 2, 2));
      }

      TEST( casual_common_predicate, or__one_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::make_or(
            []( int l, int r){ return l < r;}
         ); 

         EXPECT_TRUE( pred( 1, 2));
         EXPECT_FALSE( pred( 1, 1));
         EXPECT_FALSE( pred( 2, 1));
      }

      TEST( casual_common_predicate, or__two_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::make_or(
            []( int l, int r){ return l < r;},
            []( int l, int r){ return ( l + r) % 2 == 0;}
         ); 

         EXPECT_TRUE( pred( 2, 4));
         EXPECT_FALSE( pred( 2, 1));
         EXPECT_TRUE( pred( 2, 2));
         EXPECT_TRUE( pred( 4, 2));
      }

      TEST( casual_common_predicate, order__one_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::make_or(
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
            struct State
            {
               int a = 0;
               int b = 0;
               friend bool operator == ( const State& l, const State& r) { return l.a == r.a && l.b == r.b;}
               friend std::ostream& operator << ( std::ostream& out, const State& s) { return out << "{ a: " << s.a << ", b: " << s.b << '}';}
            };

            namespace filter 
            {
               struct base
               {
                  base( int value) : value( value) {};
                  int value = 0;
               };

               struct A : base
               {
                  using base::base;
                  bool operator () ( const State& state) const { return value == state.a;}
               };

               struct B : base
               {
                  using base::base;
                  bool operator () ( const State& state) const { return value == state.b;}
               };
            } // filter 

         /*
         namespace detail 
         {
            template< typename P>
            auto variadic_predicate( P&& predicate)
            {
               return [=]( auto&&... param){
                  return predicate( std::forward< decltype( param)>( param)...);
               };
            }
         } // detail 
         
         template< typename P>
         auto make_and( P&& predicate) { return detail::variadic_predicate( std::forward< P>( predicate));}

         template< typename P, typename... Ts>
         auto make_and( P&& predicate, Ts&&... ts)
         {
            return [=]( auto&&... param){
               return predicate( param...) && make_and( std::move( ts)...)( param...);
            };
         }
         */
         

         } // <unnamed>
      } // local

      TEST( casual_common_predicate, and__rvalue_lambdas__a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         auto pred = predicate::make_and(
            []( const local::State& s){ return s.a == 1;},
            []( const local::State& s){ return s.b == 2;}
         ); 

         auto filtered = std::get< 0>( algorithm::partition( values, pred));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( filtered == expected) << trace.compose( "filtered: ", filtered);
      }

      TEST( casual_common_predicate, and__lvalue_lambdas__a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         const auto a = []( const local::State& s){ return s.a == 1;};
         const auto b = []( const local::State& s){ return s.b == 2;};
         
         auto pred = predicate::make_and( a, b); 

         auto filtered = std::get< 0>( algorithm::partition( values, pred));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( filtered == expected) << trace.compose( "filtered: ", filtered);
      }

      TEST( casual_common_predicate, a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         auto filtered = std::get< 0>( algorithm::partition( values, local::filter::A{ 1}));
         filtered = std::get< 0>( algorithm::partition( filtered, local::filter::B{ 2}));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( filtered == expected) << trace.compose( "filtered: ", filtered);
      }

      TEST( casual_common_predicate, and__rvalue_functors__a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         auto pred = predicate::make_and(
            local::filter::A{ 1},
            local::filter::B{ 2}
         ); 

         auto filtered = std::get< 0>( algorithm::partition( values, pred));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( filtered == expected) << trace.compose( "filtered: ", filtered);
      }

      TEST( casual_common_predicate, and__lvalue_functors__a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         auto a = local::filter::A{ 1};
         auto b = local::filter::B{ 2};

         EXPECT_TRUE( a.value == 1);
         EXPECT_TRUE( b.value == 2);

         auto pred = predicate::make_and( a, b); 

         auto filtered = std::get< 0>( algorithm::partition( values, pred));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( filtered == expected) << trace.compose( "filtered: ", filtered);
      }

      
   
   } // common
} // casual