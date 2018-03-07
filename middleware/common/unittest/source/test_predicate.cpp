//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>

#include "common/unittest.h"
#include "common/predicate.h"


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
         EXPECT_TRUE( ! algorithm::is_sorted( values));

         algorithm::sort( values, pred);

         EXPECT_TRUE( algorithm::is_sorted( values));
         EXPECT_TRUE( algorithm::is_sorted( values, pred));
      }

      TEST( casual_common_predicate, order__two_lambda)
      {
         common::unittest::Trace trace;

         auto pred = predicate::make_order(
            []( int l, int r){ return l % 2 == 0 && r % 2 == 1;},
            []( int l, int r){ return l < r;}
         ); 


         std::vector< int> values{ 4, 3, 2, 1, 6, 8, 5, 7};
         EXPECT_TRUE( ! algorithm::is_sorted( values, pred));

         algorithm::sort( values, pred);

         EXPECT_TRUE( algorithm::is_sorted( values, pred)) << range::make( values);
         EXPECT_TRUE(( values == std::vector< int>{ 2, 4, 6, 8, 1, 3, 5, 7}));
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
         EXPECT_TRUE( filtered == expected) << "filtered: " << filtered;
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
         EXPECT_TRUE( filtered == expected) << "filtered: " << filtered;
      }

      TEST( casual_common_predicate, a__b)
      {
         common::unittest::Trace trace;

         auto values = std::vector< local::State>{ { 1, 2}, { 1, 3}, { 1, 2}, { 2, 2}, { 1, 2}, { 4, 7}};

         auto filtered = std::get< 0>( algorithm::partition( values, local::filter::A{ 1}));
         filtered = std::get< 0>( algorithm::partition( filtered, local::filter::B{ 2}));

         auto expected = std::vector< local::State>{ { 1, 2}, { 1, 2}, { 1, 2}};
         EXPECT_TRUE( filtered == expected) << "filtered: " << filtered;
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
         EXPECT_TRUE( filtered == expected) << "filtered: " << filtered;
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
         EXPECT_TRUE( filtered == expected) << "filtered: " << filtered;
      }

      namespace local
      {
         namespace
         {
            
            struct Value
            {
               std::string name;
               long age;
               double height;
            };

            namespace order
            {
               auto name = []( const Value& lhs, const Value& rhs){ return lhs.name < rhs.name;};
               auto age = []( const Value& lhs, const Value& rhs){ return lhs.age < rhs.age;};
               auto height = []( const Value& lhs, const Value& rhs) { return lhs.height < rhs.height;};
            } // order

            std::vector< Value> values()
            {
               return { { "Tom", 29, 1.75}, { "Charlie", 23, 1.63 }, { "Charlie", 29, 1.90}, { "Tom", 30, 1.63} };
            }

         } // <unnamed>
      } // local


      TEST( casual_common_predicate, order_name_asc)
      {
         common::unittest::Trace trace;

         auto values = local::values();

         auto sorted = algorithm::sort( values, predicate::make_order( local::order::name));

         EXPECT_TRUE( std::begin( sorted)->name == "Charlie");
      }


      TEST( casual_common_predicate, order_name_desc__age_asc)
      {
         common::unittest::Trace trace;

         auto values = local::values();

         auto sorted = algorithm::sort( values, predicate::make_order(
               predicate::inverse( local::order::name),
               local::order::age));

         EXPECT_TRUE( std::begin( sorted)->name == "Tom");
         EXPECT_TRUE( std::begin( sorted)->age == 29);
         EXPECT_TRUE( ( std::begin( sorted) + 3)->name == "Charlie");
         EXPECT_TRUE( ( std::begin( sorted) + 3)->age == 29);

      }

      TEST( casual_common_algorithm, order_age_desc__height_asc)
      {
         common::unittest::Trace trace;

         auto values = local::values();

         auto sorted = algorithm::sort( values, predicate::make_order(
               predicate::inverse( local::order::age),
               local::order::height));

         EXPECT_TRUE( ( std::begin( sorted) + 1)->age == 29);
         EXPECT_TRUE( ( std::begin( sorted) + 1)->height == 1.75);
         EXPECT_TRUE( ( std::begin( sorted) + 2)->age == 29);
         EXPECT_TRUE( ( std::begin( sorted) + 2)->height == 1.90);
      }
      
   
   } // common
} // casual