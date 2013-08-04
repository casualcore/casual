//!
//! test_arguments.cpp
//!
//! Created on: Jan 5, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/arguments.h"

#include <functional>

namespace casual
{

   namespace common
   {

      namespace local
      {
         struct conf
         {

            long someLong = 0;

            void flag() {}
         };


         long globalLong = 0;
      }

      TEST( casual_common_arguments, test_bind)
      {

         argument::internal::dispatch< argument::cardinality::Zero, decltype( std::mem_fn( &local::conf::flag))>
         someDispatch( std::mem_fn( &local::conf::flag));

         someDispatch( { "kdfjs", "sldkfjs"});

         auto memberAttribute = std::bind( &local::conf::someLong, std::placeholders::_1);
         auto globalLong = std::bind( &local::conf::someLong, std::placeholders::_1);


      }


      TEST( casual_common_arguments, blabla)
      {

         /*

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
         arguments.add(
               argument::Directive{ { "-f", "--foo"}, "foo"},
               argument::Directive{ { "-b", "--bar"}, "bar"},
               group);

         //const char* argv[] = { "-p", "-c", "arg3", "arg4" };

         //std::vector< std>

         arguments.parse( { "-f", "arg-p", "-b", "arg-b", "-p", "arg-p" });
         */

      }

   }
}



