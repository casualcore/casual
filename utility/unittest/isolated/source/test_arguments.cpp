//!
//! test_arguments.cpp
//!
//! Created on: Jan 5, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "utility/arguments.h"


namespace casual
{

   namespace utility
   {




      TEST( casual_utility_arguments, blabla)
      {

         argument::cardinality::One one;

         EXPECT_TRUE( one.min_value == 1);
         EXPECT_TRUE( one.max_value == 1);


         argument::cardinality::Range< 2, 4> range;

         argument::cardinality::Any any;

         argument::Directive directive{ { "-p", "--poo"}, "poo"};

         EXPECT_TRUE( directive.option( "-p"));
         EXPECT_FALSE( directive.option( "-a"));


         argument::Group group;
         group.add( directive, argument::Directive{ { "-c", "--clear"}, "clear"});

         EXPECT_TRUE( group.option( "-p"));
         EXPECT_TRUE( group.option( "--clear"));
         EXPECT_FALSE( group.option( "-a"));


         Arguments arguments;

         const char* argv[] = { "arg1", "arg2", "arg3", "arg4" };

         arguments.parse( sizeof( argv), argv);


      }

   }
}



