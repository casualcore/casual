//!
//! casual
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/functional.h"

namespace casual
{
   namespace common
   {
      /*
      TEST( common_functional_apply, variables_1)
      {
         int v1 = 0;
         apply( std::ref( v1), std::make_tuple( 42));

         EXPECT_TRUE( v1 == 42);
      }
      */


      TEST( common_functional_apply, lambda_arguments_0)
      {
         auto lambda = [](){
            return true;
         };

         EXPECT_TRUE( apply( lambda, std::make_tuple()));
      }

      TEST( common_functional_apply, generic_lambda_arguments_1)
      {
         auto lambda = []( auto value){
            return value;
         };

         EXPECT_TRUE( apply( lambda, std::make_tuple( 42)) == 42);
      }

      TEST( common_functional_apply, generic_lambda_arguments_2)
      {
         auto lambda = []( auto v1, auto v2){
            return v1 + v2;
         };

         EXPECT_TRUE( apply( lambda, std::make_tuple( 20, 22)) == 42);
      }

      TEST( common_functional_apply, lambda_deduce_to_tuple)
      {
         auto lambda = []( const int& v1, const int& v2){
            return v1 + v2;
         };

         auto args = traits::function< decltype( lambda)>::tuple();
         static_assert( std::tuple_size< decltype( args)>::value == 2, "");
         
         std::get< 0>( args) = 20;
         std::get< 1>( args) = 22;

         EXPECT_TRUE( apply( lambda, args) == 42);
      }

      namespace local
      {
         namespace
         {
            auto arguments_0() { return true;}
            auto arguments_1( int v1) { return v1;}
            auto arguments_2( int v1, int v2) { return v1 + v2;}
            auto arguments_3( int v1, const std::string& v2, long v3) { return std::to_string( v1) + v2 + std::to_string( v3);}
         } // <unnamed>
      } // local

      TEST( common_functional_apply, free_function_arguments_0)
      {
         EXPECT_TRUE( apply( &local::arguments_0, std::make_tuple()));
      }

      TEST( common_functional_apply, free_function_arguments_1)
      {
         EXPECT_TRUE( apply( &local::arguments_1, std::make_tuple( 42)) == 42);
      }

      TEST( common_functional_apply, free_function_arguments_2)
      {
         EXPECT_TRUE( apply( &local::arguments_2, std::make_tuple( 20, 22)) == 42);
      }
      
      TEST( common_functional_apply, free_function_deduce_to_tuple)
      {
         auto args = traits::function< decltype( &local::arguments_2)>::tuple();
         static_assert( std::tuple_size< decltype( args)>::value == 2, "");
         
         std::get< 0>( args) = 20;
         std::get< 1>( args) = 22;

         EXPECT_TRUE( apply( &local::arguments_2, args) == 42);
      }

      TEST( common_functional_apply, free_function_different_types__deduce_to_tuple)
      {
         auto args = traits::function< decltype( &local::arguments_3)>::tuple();
         static_assert( std::tuple_size< decltype( args)>::value == 3, "");
         
         std::get< 0>( args) = 20;
         std::get< 1>( args) = "some-string";
         std::get< 2>( args) = 42;

         EXPECT_TRUE( apply( &local::arguments_3, args) == "20some-string42");
      }
      namespace local
      {
         namespace
         {
            struct Object
            {
               auto arguments_0() { return true;}
               auto arguments_1( int v1) { return v1;}
               auto arguments_2( int v1, int v2) { return v1 + v2;}
            };

         } // <unnamed>
      } // local

      TEST( common_functional_apply, member_function_arguments_0)
      {
         local::Object object;
         EXPECT_TRUE( apply( &local::Object::arguments_0, std::make_tuple( std::ref( object))));
      }

      TEST( common_functional_apply, member_function_arguments_1)
      {
         local::Object object;
         EXPECT_TRUE( apply( &local::Object::arguments_1, std::make_tuple( std::ref( object), 42)) == 42);
      }

      TEST( common_functional_apply, member_function_arguments_2)
      {
         local::Object object;
         EXPECT_TRUE( apply( &local::Object::arguments_2, std::make_tuple( std::ref( object), 20, 22)) == 42);
      }

      TEST( common_functional_apply, member_function_deduce_to_tuple)
      {
         auto args = traits::function< decltype( &local::Object::arguments_2)>::tuple();
         static_assert( std::tuple_size< decltype( args)>::value == 2, "");
         
         std::get< 0>( args) = 20;
         std::get< 1>( args) = 22;

         local::Object object;

         EXPECT_TRUE( apply( &local::Object::arguments_2, std::tuple_cat( std::tie( object), args)) == 42);
         using namespace std::placeholders;
         EXPECT_TRUE( apply( std::bind( &local::Object::arguments_2, object, _1, _2), args) == 42);
      }

   } // common
} // casual