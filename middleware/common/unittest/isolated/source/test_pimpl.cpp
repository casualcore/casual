//!
//! casual 
//!

#include "common/unittest.h"


#include "common/pimpl.h"


namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            struct Value
            {
               Value() = default;
               Value( long l, std::string s) : some_long( l), some_string( std::move( s)) {}

               long some_long = 42;
               std::string some_string = "42";
            };

         } // <unnamed>
      } // local


      TEST( common_move_pimpl, default_instansiation)
      {
         CASUAL_UNITTEST_TRACE();

         move::basic_pimpl< local::Value> value;

         EXPECT_TRUE( value->some_long == 42);
         EXPECT_TRUE( value->some_string == "42");
      }

      TEST( common_move_pimpl, instansiation_ctor)
      {
         CASUAL_UNITTEST_TRACE();

         move::basic_pimpl< local::Value> value{ 666, "666"};

         EXPECT_TRUE( value->some_long == 666);
         EXPECT_TRUE( value->some_string == "666");
      }

      TEST( common_move_pimpl, move)
      {
         CASUAL_UNITTEST_TRACE();

         move::basic_pimpl< local::Value> source{ 666, "666"};

         auto result = std::move( source);

         EXPECT_TRUE( result->some_long == 666);
         EXPECT_TRUE( result->some_string == "666");

         EXPECT_TRUE( static_cast< bool>( result) == true);
         EXPECT_TRUE( static_cast< bool>( source) == false);
      }


      TEST( common_pimpl, default_instansiation)
      {
         CASUAL_UNITTEST_TRACE();

         basic_pimpl< local::Value> value;

         EXPECT_TRUE( value->some_long == 42);
         EXPECT_TRUE( value->some_string == "42");
      }

      TEST( common_pimpl, instansiation_ctor)
      {
         CASUAL_UNITTEST_TRACE();

         basic_pimpl< local::Value> value{ 666, "666"};

         EXPECT_TRUE( value->some_long == 666);
         EXPECT_TRUE( value->some_string == "666");
      }


      TEST( common_pimpl, deep_copy)
      {
         CASUAL_UNITTEST_TRACE();

         basic_pimpl< local::Value> a{ 666, "666"};

         auto b = a;

         EXPECT_TRUE( a->some_long == 666);
         EXPECT_TRUE( a->some_string == "666");
         EXPECT_TRUE( b->some_long == 666);
         EXPECT_TRUE( b->some_string == "666");

         // make sure we got a deep copy
         b->some_long = 42;
         b->some_string = "42";

         EXPECT_TRUE( a->some_long == 666);
         EXPECT_TRUE( a->some_string == "666");
         EXPECT_TRUE( b->some_long == 42);
         EXPECT_TRUE( b->some_string == "42");
      }

      TEST( common_pimpl, move)
      {
         CASUAL_UNITTEST_TRACE();

         basic_pimpl< local::Value> source{ 666, "666"};

         auto result = std::move( source);

         EXPECT_TRUE( result->some_long == 666);
         EXPECT_TRUE( result->some_string == "666");

         EXPECT_TRUE( static_cast< bool>( result) == true);
         EXPECT_TRUE( static_cast< bool>( source) == false);
      }

   } // common
} // casual
