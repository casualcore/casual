//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/argument.h"

namespace casual
{
   using namespace common;

   namespace local
   {
      namespace
      {
         
         auto parse_complete( std::vector< argument::Option> options, std::vector< std::string_view> arguments)
         {
            arguments.insert( std::begin( arguments), argument::reserved::name::completion);

            std::ostringstream out;
            auto guard = common::unittest::capture::standard::out( out);
            argument::parse( "", options, arguments);

            return string::split( std::move( out).str(), '\n');
         }
         
      } // <unnamed>
   } // local

   TEST( argument_complete, basic_completer)
   {
      unittest::Trace trace;

      {
         auto invoke = [](){};

         using invoke_type = decltype( invoke);
         auto completer = argument::detail::option::default_completer< invoke_type>{};

         EXPECT_TRUE( completer( false, {}).empty()) <<  CASUAL_NAMED_VALUE( completer( false, {}));
      }

      {
         auto invoke = []( bool a){};

         using invoke_type = decltype( invoke);
         auto completer = argument::detail::option::default_completer< invoke_type>{};

         EXPECT_TRUE( completer( false, {}).at( 0) == "<value>") << CASUAL_NAMED_VALUE( completer( false, {}));
      }

   }

   TEST( argument_complete, simple)
   {
      unittest::Trace trace;

      struct
      {
         long a{};
         long b{};
         std::optional< long> c;

      } state;

      auto options = std::vector< argument::Option>{ 
         argument::Option{ std::tie( state.a), { "-a"}, ""},
         argument::Option{ std::tie( state.b), { "-b"}, ""},
         argument::Option{ std::tie( state.c), { "-c"}, ""},
      };

      {
         auto output = local::parse_complete( options, {});
         ASSERT_TRUE( output.size() == 3) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a");
         EXPECT_TRUE( output.at( 1) == "-b");
         EXPECT_TRUE( output.at( 2) == "-c");
      }

      {
         auto output = local::parse_complete( options, { "-b"});
         EXPECT_TRUE( output.size() == 1);
         EXPECT_TRUE( output.at( 0) == argument::reserved::name::suggestions);
      }

      {
         auto output = local::parse_complete( options, { "-b", "1"});
         EXPECT_TRUE( output.size() == 2) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a");
         EXPECT_TRUE( output.at( 1) == "-c");
      }

      {
         auto output = local::parse_complete( options, { "-c"});
         EXPECT_TRUE( output.size() == 3) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == argument::reserved::name::suggestions);
         EXPECT_TRUE( output.at( 1) == "-a");
         EXPECT_TRUE( output.at( 2) == "-b");
      }
   }


   TEST( argument_complete, suboptions)
   {
      unittest::Trace trace;

      struct
      {
         long a{};
         long b{};
         std::optional< long> c;
         std::vector< std::tuple< long, long>> d;
         std::tuple< long, std::optional< long>> e;

      } state;

      auto options = std::vector< argument::Option>{ 
         argument::Option{ std::tie( state.a), { "-a"}, ""}( {     
            argument::Option{ std::tie( state.b), { "-b"}, ""}( {
               argument::Option{ std::tie( state.c), { "-c"}, ""},
            })
         }),
         argument::Option{ std::tie( state.d), { "-d"}, ""}( {
            argument::Option{ std::tie( state.e), { "-e"}, ""}
         })
      };
      
      {
         auto output = local::parse_complete( options, {});
         ASSERT_TRUE( output.size() == 2) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a");
         EXPECT_TRUE( output.at( 1) == "-d");
      }

      {
         auto output = local::parse_complete( options, { "-a", "1", "-b", "2"});
         EXPECT_TRUE( output.size() == 2) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-c");
         EXPECT_TRUE( output.at( 1) == "-d");
      }

      {
         // value completion on -e
         auto output = local::parse_complete( options, { "-d", "1", "2", "-e"});
         ASSERT_TRUE( output.size() == 1) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == argument::reserved::name::suggestions);
      }

      {
         // value completion on -e -> filled -> expect only "-a" since "-e" and "-d" are "used" and not part of the completion any more
         auto output = local::parse_complete( options, { "-d", "1", "2", "-e", "3", "4"});
         ASSERT_TRUE( output.size() == 1) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a") << CASUAL_NAMED_VALUE( output);
      }
   }

   TEST( argument_complete, suboptions_cardinality)
   {
      unittest::Trace trace;

      struct
      {
         long a{};
         long a1{};
         long a2{};
         long b{};
         long b1{};
         long b2{};
         long b3{};
         long b4{};

      } state;

      auto options = std::vector< argument::Option>{ 
         argument::Option{ std::tie( state.a), { "-a"}, ""}( {     
            argument::Option{ std::tie( state.a1), { "-a1"}, ""},
            argument::Option{ std::tie( state.a2), { "-a2"}, ""},
         }, argument::cardinality::one()),
         argument::Option{ std::tie( state.b), { "-b"}, ""}( {
            argument::Option{ std::tie( state.b1), { "-b1"}, ""},
            argument::Option{ std::tie( state.b2), { "-b2"}, ""},
            argument::Option{ std::tie( state.b3), { "-b3"}, ""},
            argument::Option{ std::tie( state.b4), { "-b4"}, ""}
         }, argument::cardinality::range( 2, 3)),
      };

      // completion for -a, expect only suboptions -a1 and -a2
      {
         auto output = local::parse_complete( options, { "-a", "42"});
         ASSERT_TRUE( output.size() == 2) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a1");
         EXPECT_TRUE( output.at( 1) == "-a2");
      }

      // completion for -a 42 -a1 43, expect suboptions to be exhausted -> -b
      {
         auto output = local::parse_complete( options, { "-a", "42", "-a1", "43"});
         ASSERT_TRUE( output.size() == 1) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-b");
      }

      // completion for -b 42 -b1 42, expect -b2, -b3, -b4
      {
         auto output = local::parse_complete( options, { "-b", "42", "-b1", "42"});
         ASSERT_TRUE( output.size() == 3) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-b2");
         EXPECT_TRUE( output.at( 1) == "-b3");
         EXPECT_TRUE( output.at( 2) == "-b4");
      }

      // completion for -b 42 -b1 42 -b3 42, expect -b2, -b4, -a
      {
         auto output = local::parse_complete( options, { "-b", "42", "-b1", "42", "-b3", "42"});
         ASSERT_TRUE( output.size() == 3) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-b2");
         EXPECT_TRUE( output.at( 1) == "-b4");
         EXPECT_TRUE( output.at( 2) == "-a");
      }

      // completion for -b 42 -b1 42 -b3 42 -b2 42, expect suboptions to be exhausted -> -a
      {
         auto output = local::parse_complete( options, { "-b", "42", "-b1", "42", "-b3", "42", "-b2", "42"});
         ASSERT_TRUE( output.size() == 1) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a");
      }
   }

   TEST( argument_complete, suboptions_cardinality_suboptions_has_option_cardinality_any)
   {
      unittest::Trace trace;

      struct
      {
         long a{};
         long a1{};
         long a2{};
         long a3{};
         long b{};
         long b1{};
         long b2{};

      } state;

      auto options = std::vector< argument::Option>{ 
         argument::Option{ std::tie( state.a), { "-a"}, ""}( {     
            argument::Option{ std::tie( state.a1), { "-a1"}, ""}( argument::cardinality::fixed( 2)),
            argument::Option{ std::tie( state.a2), { "-a2"}, ""}( argument::cardinality::zero_one()),
            argument::Option{ std::tie( state.a3), { "-a3"}, ""}( argument::cardinality::any()),
         }, argument::cardinality::fixed( 2)),
         argument::Option{ std::tie( state.b), { "-b"}, ""}( {
            argument::Option{ std::tie( state.b1), { "-b1"}, ""},
            argument::Option{ std::tie( state.b2), { "-b2"}, ""},
         }, argument::cardinality::one()),
      };

      // completion for -a 42, -a1 42, -a1 43 -> -a1 exhausted, suboptions cardinality fixed 2 not satisfied -> -a2, -a3
      {
         auto output = local::parse_complete( options, { "-a", "42", "-a1", "42", "-a1", "43"});
         ASSERT_TRUE( output.size() == 2) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a2");
         EXPECT_TRUE( output.at( 1) == "-a3");
      }

      // completion for -a 42, -a1 42, -a2 42 -> 
      // * suboptions cardinality fixed 2 satisfied -> -a1 and -a2 is "locked"
      // * -a2 is exhausted
      // * -a1 is not satisfied
      // expect only a1
      {
         auto output = local::parse_complete( options, { "-a", "42", "-a1", "42", "-a2", "42"});
         ASSERT_TRUE( output.size() == 1) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a1");

      }

   }

   TEST( argument_complete, immediate_flags)
   {
      unittest::Trace trace;

      struct State
      {
         long a{};
         std::vector< long> b;

         bool f1 = false;
         bool f2 = false;
      };

      static auto callback = []( State& state)
      { 
         return [ &state]( long a, std::vector< long> b)
         {
            state.a = a;
            state.b = b;
         };
      };

      auto options = []( State& state)
      {
         return std::vector< argument::Option>{ 
            argument::Option{ callback( state), { "-a"}, ""}( {     
               argument::Option{ argument::option::flag( state.f1), { "-f1"}, ""},
               argument::Option{ argument::option::flag( state.f2), { "-f2"}, ""}
            })};
      };
      
      {
         State state;
         auto output = local::parse_complete( options( state), { "-a", "42"});
         ASSERT_TRUE( output.size() == 3) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == argument::reserved::name::suggestions);
         EXPECT_TRUE( output.at( 1) == "-f1");
         EXPECT_TRUE( output.at( 2) == "-f2");
      }

      {
         State state;
         auto output = local::parse_complete( options( state), { "-a", "-f1", "42"});
         ASSERT_TRUE( output.size() == 2) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == argument::reserved::name::suggestions);
         EXPECT_TRUE( output.at( 1) == "-f2");
      }

      {
         State state;
         auto output = local::parse_complete( options( state), { "-a", "-f1", "-f2", "42"});
         ASSERT_TRUE( output.size() == 1) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == argument::reserved::name::suggestions);
      }


      {
         State state;
         auto output = local::parse_complete( options( state), { "-a", "42", "-f1"});
         ASSERT_TRUE( output.size() == 1) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-f2");
      }
   }


   TEST( argument_complete, suboptions_flags)
   {
      unittest::Trace trace;

      auto flag = []()
      {
      };

      auto options = std::vector< argument::Option>{ 
         argument::Option{ flag, { "-a"}, ""}( {     
            argument::Option{ flag, { "-b"}, ""}( {
               argument::Option{ flag, { "-c"}, ""},
            })
         }),
         argument::Option{ flag, { "-d"}, ""}( {
            argument::Option{ flag, { "-e"}, ""}
         })
      };

      {
         auto output = local::parse_complete( options, {});
         ASSERT_TRUE( output.size() == 2) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a");
         EXPECT_TRUE( output.at( 1) == "-d");
      }

      {
         auto output = local::parse_complete( options, { "-a", "-b"});
         ASSERT_TRUE( output.size() == 2) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-c");
         EXPECT_TRUE( output.at( 1) == "-d");
      }

      {
         auto output = local::parse_complete( options, { "-d", "-e"});
         ASSERT_TRUE( output.size() == 1) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a");
      }
   }

   TEST( argument_complete, option_with_flag)
   {
      unittest::Trace trace;

      auto callback = []( long a, std::optional< long> b)
      {
      };

      auto flag = [](){};

      auto options = std::vector< argument::Option>{ 
         argument::Option{ callback, { "-a"}, ""}( {     
            argument::Option{ flag, { "-b"}, ""}
         }),
         argument::Option{ callback, { "-c"}, ""}
      };

      {
         auto output = local::parse_complete( options, {});
         ASSERT_TRUE( output.size() == 2) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-a");
         EXPECT_TRUE( output.at( 1) == "-c");
      }

      {
         auto output = local::parse_complete( options, { "-a"});
         ASSERT_TRUE( output.size() == 1) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == argument::reserved::name::suggestions);
      }

      {
         auto output = local::parse_complete( options, { "-a", "1"});
         ASSERT_TRUE( output.size() == 3) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == argument::reserved::name::suggestions);
         EXPECT_TRUE( output.at( 1) == "-b");
         EXPECT_TRUE( output.at( 2) == "-c");
      }

      {
         auto output = local::parse_complete( options, { "-a", "1", "2"});
         ASSERT_TRUE( output.size() == 2) << CASUAL_NAMED_VALUE( output);
         EXPECT_TRUE( output.at( 0) == "-b");
         EXPECT_TRUE( output.at( 1) == "-c");
      }
   }   
} // casual
