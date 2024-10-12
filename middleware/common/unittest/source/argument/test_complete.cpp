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


   TEST( argument_complete, nested_options)
   {
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

   TEST( argument_complete, immediate_flags)
   {
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


   TEST( argument_complete, nested_flags)
   {
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