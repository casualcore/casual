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

            void setString( const std::string& value)
            {
               string_value = value;
            }

            void setLong( long value)
            {
               long_value = value;
            }

            void setVectorString( const std::vector< std::string>& value)
            {
               vector_string_value = value;
            }

            void setVectorLong( const std::vector< long>& value)
            {
               vector_long_value = value;
            }

            bool called = false;

            std::string string_value;
            long long_value = 0;
            std::vector< std::string> vector_string_value;
            std::vector< long> vector_long_value;
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

         auto dispatch1 = argument::internal::make( argument::cardinality::One(), conf, &local::Conf::setString);

         dispatch1( values);


         EXPECT_TRUE( conf.string_value == "234");

         auto dispatch2 = argument::internal::make( argument::cardinality::One(), conf, &local::Conf::setLong);

         dispatch2( values);

         EXPECT_TRUE( conf.long_value == 234);
      }


      TEST( casual_common_arguments, directive_deduced_cardinality__member_function_void)
      {

         local::Conf conf;


         Arguments arguments;

         arguments.add(
               argument::directive( { "-f", "--foo"}, "some foo stuff", conf, &local::Conf::flag)
         );

         EXPECT_FALSE( conf.called );

         arguments.parse( { "-f"});

         EXPECT_TRUE( conf.called);

      }

      TEST( casual_common_arguments, directive_deduced_cardinality__member_function_string)
      {

         local::Conf conf;

         Arguments arguments;

         arguments.add(
               argument::directive( { "-f", "--foo"}, "some foo stuff", conf, &local::Conf::setString)
         );

         arguments.parse( { "-f" ,"someValue"});

         EXPECT_TRUE( conf.string_value == "someValue");


      }

      TEST( casual_common_arguments, directive_deduced_cardinality__member_function_long)
      {

         local::Conf conf;

         Arguments arguments;

         arguments.add(
               argument::directive( { "-f", "--foo"}, "some foo stuff", conf, &local::Conf::setLong)
         );

         arguments.parse( { "-f" ,"42"});

         EXPECT_TRUE( conf.long_value == 42);

      }


      TEST( casual_common_arguments, directive_deduced_cardinality__member_function_vector_string)
      {

         local::Conf conf;

         Arguments arguments;

         arguments.add(
               argument::directive( { "-f", "--foo"}, "some foo stuff", conf, &local::Conf::setVectorString)
         );

         arguments.parse( { "-f" ,"42", "666", "777"});

         EXPECT_TRUE( conf.vector_string_value.size() == 3);

      }

      TEST( casual_common_arguments, directive_deduced_cardinality__member_function_vector_long)
      {

         local::Conf conf;

         Arguments arguments;

         arguments.add(
               argument::directive( { "-f", "--foo"}, "some foo stuff", conf, &local::Conf::setVectorLong)
         );

         arguments.parse( { "-f" ,"42", "666", "777"});

         EXPECT_TRUE( conf.vector_long_value.size() == 3);

      }

      TEST( casual_common_arguments, directive_any_cardinality__member_function_vector_long__expect_3)
      {

         local::Conf conf;

         Arguments arguments;

         arguments.add(
               argument::directive( argument::cardinality::Any(), { "-f", "--foo"}, "some foo stuff", conf, &local::Conf::setVectorLong)
         );

         arguments.parse( { "-f" ,"42", "666", "777"});

         EXPECT_TRUE( conf.vector_long_value.size() == 3);

      }

      TEST( casual_common_arguments, directive_any_cardinality__member_function_vector_long__expect_0)
      {

         local::Conf conf;

         Arguments arguments;

         arguments.add(
               argument::directive( argument::cardinality::Any(), { "-f", "--foo"}, "some foo stuff", conf, &local::Conf::setVectorLong)
         );

         arguments.parse( { "-f" });

         EXPECT_TRUE( conf.vector_long_value.size() == 0);

      }

      TEST( casual_common_arguments, directive_fixed_3_cardinality__member_function_vector_long__expect_3)
      {

         local::Conf conf;

         Arguments arguments;

         arguments.add(
               argument::directive( argument::cardinality::Fixed< 3>(), { "-f", "--foo"}, "some foo stuff", conf, &local::Conf::setVectorLong)
         );

         arguments.parse( { "-f" ,"42", "666", "777"});

         EXPECT_TRUE( conf.vector_long_value.size() == 3);

      }

   }
}



