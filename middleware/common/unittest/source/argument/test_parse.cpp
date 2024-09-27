//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/argument.h"

namespace casual
{

   TEST( argument_parse, empty)
   {

      argument::parse( "", {}, {});

   }

   TEST( argument_parse, simple)
   {
      struct
      {
         long a{};
         long b{};

      } state;

      argument::parse( "", { 
            argument::Option{ std::tie( state.a), { "-a"}, ""},
            argument::Option{ std::tie( state.b), { "-b"}, ""},
         }, 
         { "-b", "2", "-a", "1"});

      EXPECT_TRUE( state.a == 1);
      EXPECT_TRUE( state.b == 2);

   }

   TEST( argument_parse, simple_nested)
   {
      
      std::vector< std::string> invoked;

      auto callback = [ &invoked]( std::string name)
      {
         return [ &invoked, name]()
         {
            invoked.push_back( name);
         };
      };


      argument::parse( "", { 
            argument::Option{ callback( "a"), { "-a"}, ""}( {
               argument::Option{ callback( "a1"), { "-a1"}, ""}
            }),
            argument::Option{ callback( "b"), { "-b"}, ""}( {
               argument::Option{ callback( "b1"), { "-b1"}, ""}
            }),
         }, 
         { "-b", "-b1", "-a", "-a1"});

      EXPECT_TRUE(( invoked == std::vector< std::string>{ "b", "b1", "a", "a1" }));
   }

   TEST( argument_parse, invalid_value_cardinality)
   {
      struct
      {
         long a{};
         long b{};
      } state;

      ASSERT_CODE( argument::parse( "", { 
            argument::Option{ std::tie( state.a), { "-a"}, ""},
         }, 
         { "-a", "2", "3", "4"}), common::code::casual::invalid_argument);

      ASSERT_CODE( argument::parse( "", { 
            argument::Option{ std::tie( state.b), { "-b"}, ""},
         }, 
         { "-b"}), common::code::casual::invalid_argument);
   }

   TEST( argument_parse, invalid_option_cardinality)
   {
      struct
      {
         long a{};
      } state;

      ASSERT_CODE( argument::parse( "", { 
            argument::Option{ std::tie( state.a), { "-a"}, ""}
         }, 
         { "-a", "2", "-a", "4"}), common::code::casual::invalid_argument);

      ASSERT_CODE( argument::parse( "", { 
            argument::Option{ std::tie( state.a), { "-a"}, ""}( argument::cardinality::one()),
         }, 
         {}), common::code::casual::invalid_argument);

      ASSERT_CODE( argument::parse( "", { 
            argument::Option{ std::tie( state.a), { "-a"}, ""}( argument::cardinality::one()),
         }, 
         { "-a", "2", "-a", "3"}), common::code::casual::invalid_argument);

   }

   TEST( argument_parse, help)
   {
      struct
      {
         long a{};
         long b{};
         long c{};
         long d{};
         long e{};
      } state;


      auto options = std::vector{
         argument::Option{ std::tie( state.a), { "-a"}, "description for option\n\nmulti\nline"}( {
               argument::Option{ std::tie( state.b), { "-b"}, "description for option\n\nmulti\nline"}
            }),
            argument::Option{ std::tie( state.c), { "-c"}, "description for option\n\nmulti\nline"}( {
               argument::Option{ std::tie( state.d), { "-d"}, "description for option\n\nmulti\nline"}( {
                  argument::Option{ std::tie( state.e), { "-e"}, "description for -e"}
               })
            })
         };
      
      {
         constexpr std::string_view expected = R"(NAME
   test-casual-common-isolated

DESCRIPTION

   multi
   line
   
   description
   
   bla bla bla
   foo bar

OPTIONS

   -a [0..1]
      description for option
      
      multi
      line

      SUB OPTIONS:

         -b [0..1]
            description for option
            
            multi
            line

   -c [0..1]
      description for option
      
      multi
      line

      SUB OPTIONS:

         -d [0..1]
            description for option
            
            multi
            line

            SUB OPTIONS:

               -e [0..1]
                  description for -e

)";
         
         std::ostringstream out;
         auto capture = common::unittest::capture::standard::out( out);
         argument::parse( "multi\nline\n\ndescription\n\nbla bla bla\nfoo bar", options, { "--help"});

         EXPECT_TRUE( out.str() == expected) << "out: \n" << out.str() << "expected:\n" << expected;

      }

      {
         constexpr std::string_view expected = R"(-e [0..1]
   description for -e

)";

         std::ostringstream out;
         auto capture = common::unittest::capture::standard::out( out);
         argument::parse( "", options, { "-c", "-d", "-e", "--help"});

         EXPECT_TRUE( out.str() == expected) << out.str();
      }

   }
   
} // casual