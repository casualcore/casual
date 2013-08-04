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
         struct Conf
         {

            long someLong = 0;

            void flag()
            {
               called = true;
            }

            void one( const std::string& value)
            {
               one_value = value;
            }

            void setLong( long value)
            {
               one_long_value = value;
            }

            bool called = false;

            std::string one_value;
            long one_long_value = 0;
         };


         long globalLong = 0;
      }

      TEST( casual_common_arguments, test_bind)
      {
         local::Conf conf;


         auto dispatch0 = argument::internal::make( argument::cardinality::Zero(), conf, &local::Conf::flag);


         std::vector< std::string> values{ "234", "two", "three"};

         dispatch0( values);

         EXPECT_TRUE( conf.called);

         using namespace std::placeholders;

         auto dispatch1 = argument::internal::make( argument::cardinality::One(), conf, &local::Conf::one);

         dispatch1( values);


         EXPECT_TRUE( conf.one_value == "234");

         auto dispatch2 = argument::internal::make( argument::cardinality::One(), conf, &local::Conf::setLong);

         dispatch2( values);

         EXPECT_TRUE( conf.one_long_value == 234);
      }


      TEST( casual_common_arguments, directive_zero_cardinality_member_function)
      {

         local::Conf conf;


         Arguments arguments;

         arguments.add(
               argument::directive( argument::cardinality::Zero(), { "-f", "--foo"}, "some foo stuff", conf, &local::Conf::flag)
         );

         EXPECT_FALSE( conf.called );

         arguments.parse( { "-f"});

         EXPECT_TRUE( conf.called);

      }

      TEST( casual_common_arguments, directive_one_cardinality_member_function)
      {

         local::Conf conf;

         Arguments arguments;

         arguments.add(
               argument::directive( { "-f", "--foo"}, "some foo stuff", conf, &local::Conf::one)
         );

         arguments.parse( { "-f" ,"someValue"});

         EXPECT_TRUE( conf.one_value == "someValue");


      }

   }
}



