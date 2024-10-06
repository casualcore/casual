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

   TEST( argument_option, traits_cardinality)
   {
      unittest::Trace trace;

      EXPECT_TRUE( argument::detail::value::traits< long>::cardinality() == argument::cardinality::one());

      static_assert( argument::detail::value::traits< std::tuple< long, short>>::cardinality() == argument::cardinality::fixed( 2));
      EXPECT_TRUE(( argument::detail::value::traits< std::tuple< long, short>>::cardinality() == argument::cardinality::fixed( 2)));

      static_assert( argument::detail::value::traits< std::tuple< long&, short&>>::cardinality() == argument::cardinality::fixed( 2));
      EXPECT_TRUE(( argument::detail::value::traits< std::tuple< long&, short&>>::cardinality() == argument::cardinality::fixed( 2)));

      static_assert( argument::detail::value::traits< std::tuple< long, std::optional< short>>>::cardinality() == argument::cardinality::range( 1, 2));
      EXPECT_TRUE(( argument::detail::value::traits< std::tuple< long, std::optional< short>>>::cardinality() == argument::cardinality::range( 1, 2)));

      static_assert( argument::detail::value::traits< std::tuple< long, std::tuple< int, std::optional< std::tuple< int, int>>>>>::cardinality() == argument::cardinality::range( 2, 4));
      EXPECT_TRUE(( argument::detail::value::traits< std::tuple< long, std::tuple< int, std::optional< std::tuple< int, int>>>>>::cardinality() == argument::cardinality::range( 2, 4)));

   }


   TEST( argument_option, cardinality)
   {
      struct
      {
         long a{};

      } state;

      auto option = argument::Option{ std::tie( state.a), {"-a"}, "description"}( argument::cardinality::one());

      EXPECT_TRUE( option.cardinality() == argument::cardinality::one());
   }

   TEST( argument_option, tuple_assign)
   {
      unittest::Trace trace;

      struct
      {
         long l{};
         std::string s{};

      } state;

      auto option = argument::Option{ std::tie( state.l, state.s), argument::option::Names{ {"-a"}}, "description"};
      EXPECT_TRUE( option == "-a");
      EXPECT_TRUE( option != "-b");

      std::vector< std::string_view> arguments{ "42", "casual"};
      auto assigned = option.assign( "-a", arguments);
      EXPECT_TRUE( option.usage() == 1);
      EXPECT_TRUE( ! assigned);
      EXPECT_TRUE( state.l == 42);
      EXPECT_TRUE( state.s == "casual") << state.s;
   }
 

   TEST( argument_option, callback)
   {
      unittest::Trace trace;

      struct
      {
         long a{};
         std::string b{};

      } state;

      auto callback = [ &state]( long a, std::string b)
      {
         state.a = a;
         state.b = b;
      };


      auto option = argument::Option{ callback, argument::option::Names{ {"-a"}}, "description"};
      std::vector< std::string_view> arguments{ "42", "casual"};
      auto assigned = option.assign( "-a", arguments);
      EXPECT_TRUE( option.usage() == 1);
      ASSERT_TRUE( assigned);

      EXPECT_TRUE( state.a == long{});
      EXPECT_TRUE( state.b.empty()) << state.b;
      assigned->invoke();
      EXPECT_TRUE( state.a == 42);
      EXPECT_TRUE( state.b == "casual") << state.b;
   }

   TEST( argument_option, preemptive_callback)
   {
      unittest::Trace trace;

      struct State
      {
         bool a = false;
      };

      auto preemptive = []( State& state)
      {
         return [ &state]()
         {
            state.a = true;
            return argument::option::invoke::preemptive{};
         };
      };

      State state;
      auto options = std::vector{ 
         argument::Option{ preemptive( state), { "-a"}, "description"},
      };

      std::vector< std::string_view> arguments{ "-a"};
      auto assigned = argument::detail::assign( options, arguments);
      EXPECT_TRUE( assigned.at( 0).phase() == decltype( assigned.at( 0).phase())::preemptive);

   }

   TEST( argument_option, preemptive_callbacks)
   {
      unittest::Trace trace;

      struct State
      {
         bool a = false;
         bool b = false;
      };

      auto preemptive = []( State& state)
      {
         return [ &state]()
         {
            state.a = true;
            return argument::option::invoke::preemptive{};
         };
      };

      auto regular = []( State& state)
      {
         return [ &state]()
         {
            // we expect the preemptive to have been assigned.
            EXPECT_TRUE( state.a);
            state.b = true;
         };
      };

      State state;
      auto options = std::vector{ 
         argument::Option{ regular( state), { "-a"}, "regular"},
         argument::Option{ preemptive( state), { "-b"}, "preemptive"},
      };

      std::vector< std::string_view> arguments{ "-a", "-b"};
      auto assigned = argument::detail::assign( options, arguments);
      auto [ first, second] = common::algorithm::stable::partition( assigned, []( auto& assigned){ return assigned.phase() == decltype( assigned.phase())::preemptive;});

      EXPECT_TRUE( first.at( 0).phase() == decltype( first.at( 0).phase())::preemptive);
      EXPECT_TRUE( second.at( 0).phase() == decltype( first.at( 0).phase())::regular);
   }


   TEST( argument_option, nested_tuple_options)
   {
      unittest::Trace trace;

      constexpr static auto create = []( auto tie, auto name, std::vector< argument::Option> suboptions = {})
      {
         return argument::Option{ tie, argument::option::Names{ { name}}, ""}( std::move( suboptions));
      };


      struct State
      {
         long a{};
         long b{};
         long c{};
         long d{};
         long e{};
      };

/*
      {
         State state;
         auto options = std::vector{ create( std::tie( state.a), "-a", std::vector< argument::Option>{ 
               create( std::tie( state.b), "-b"),
               create( std::tie( state.c), "-c", std::vector< argument::Option>{ 
                  create( std::tie( state.d), "-d"),
                  create( std::tie( state.e), "-e"),
               })
            }
         )};

         std::vector< std::string_view> arguments{ "-a", "1", "-b", "2", "-c", "3", "-d", "4", "-e", "5"};
         argument::detail::assign( options, arguments);

         EXPECT_TRUE( state.a == 1);
         EXPECT_TRUE( state.b == 2);
         EXPECT_TRUE( state.c == 3);
         EXPECT_TRUE( state.d == 4);
         EXPECT_TRUE( state.e == 5);
      }
     */
      {
         State state;
         auto options = std::vector{ 
               create( std::tie( state.a), "-a", std::vector< argument::Option>{ 
                  create( std::tie( state.b), "-b")}),
               create( std::tie( state.c), "-c", std::vector< argument::Option>{ 
                  create( std::tie( state.d), "-d"),
                  create( std::tie( state.e), "-e"),
               })
            };

         std::vector< std::string_view> arguments{ "-c", "3", "-d", "4", "-e", "5", "-a", "1", "-b", "2"};
         argument::detail::assign( options, arguments);

         EXPECT_TRUE( state.a == 1);
         EXPECT_TRUE( state.b == 2);
         EXPECT_TRUE( state.c == 3);
         EXPECT_TRUE( state.d == 4);
         EXPECT_TRUE( state.e == 5);
      }

      {
         State state;
         auto options = std::vector{ 
               create( std::tie( state.a), "-a", std::vector< argument::Option>{ 
                  create( std::tie( state.b), "-b", std::vector< argument::Option>{ 
                     create( std::tie( state.c), "-c", std::vector< argument::Option>{ 
                        create( std::tie( state.d), "-d", std::vector< argument::Option>{ 
                           create( std::tie( state.e), "-e")
            })})})})};

         std::vector< std::string_view> arguments{ "-a", "1", "-b", "2", "-c", "3", "-d", "4", "-e", "5"};
         argument::detail::assign( options, arguments);

         EXPECT_TRUE( state.a == 1);
         EXPECT_TRUE( state.b == 2) << state.b;
         EXPECT_TRUE( state.c == 3);
         EXPECT_TRUE( state.d == 4);
         EXPECT_TRUE( state.e == 5);
      }
   }


   TEST( argument_option, nested_callback_options)
   {
      unittest::Trace trace;

      constexpr static auto create = []( long& value, auto name, std::vector< argument::Option> suboptions = {})
      {
         auto assign_value = [ &value]( long argument){ value = argument;};

         return argument::Option{ assign_value, argument::option::Names{ { name}}, ""}( std::move( suboptions));
      };


      struct State
      {
         long a{};
         long b{};
         long c{};
         long d{};
         long e{};
      };

      {
         State state;
         auto options = std::vector{ create( state.a, "-a", std::vector< argument::Option>{ 
               create( state.b, "-b"),
               create( state.c, "-c", std::vector< argument::Option>{ 
                  create( state.d, "-d"),
                  create( state.e, "-e"),
               })
            }
         )};

         std::vector< std::string_view> arguments{ "-a", "1", "-b", "2", "-c", "3", "-d", "4", "-e", "5"};
         auto assigned = argument::detail::assign( options, arguments);
         EXPECT_TRUE( assigned.size() == 5);
         std::ranges::for_each( assigned, std::mem_fn( &argument::detail::option::Assigned::invoke));


         EXPECT_TRUE( state.a == 1);
         EXPECT_TRUE( state.b == 2);
         EXPECT_TRUE( state.c == 3);
         EXPECT_TRUE( state.d == 4);
         EXPECT_TRUE( state.e == 5);
      }

      {
         State state;
         auto options = std::vector{ 
               create( state.a, "-a", std::vector< argument::Option>{ 
                  create( state.b, "-b")}),
               create( state.c, "-c", std::vector< argument::Option>{ 
                  create( state.d, "-d"),
                  create( state.e, "-e"),
               })
            };

         std::vector< std::string_view> arguments{ "-c", "3", "-d", "4", "-e", "5", "-a", "1", "-b", "2"};
         auto assigned = argument::detail::assign( options, arguments);
         EXPECT_TRUE( assigned.size() == 5);
         std::ranges::for_each( assigned, std::mem_fn( &argument::detail::option::Assigned::invoke));

         EXPECT_TRUE( state.a == 1);
         EXPECT_TRUE( state.b == 2);
         EXPECT_TRUE( state.c == 3);
         EXPECT_TRUE( state.d == 4);
         EXPECT_TRUE( state.e == 5);
      }

      {
         State state;
         auto options = std::vector{ 
               create( state.a, "-a", std::vector< argument::Option>{ 
                  create( state.b, "-b", std::vector< argument::Option>{ 
                     create( state.c, "-c", std::vector< argument::Option>{ 
                        create( state.d, "-d", std::vector< argument::Option>{ 
                           create( state.e, "-e")
            })})})})};

         std::vector< std::string_view> arguments{ "-a", "1", "-b", "2", "-c", "3", "-d", "4", "-e", "5"};
         auto assigned = argument::detail::assign( options, arguments);
         EXPECT_TRUE( assigned.size() == 5);
         std::ranges::for_each( assigned, std::mem_fn( &argument::detail::option::Assigned::invoke));

         EXPECT_TRUE( state.a == 1);
         EXPECT_TRUE( state.b == 2) << state.b;
         EXPECT_TRUE( state.c == 3);
         EXPECT_TRUE( state.d == 4);
         EXPECT_TRUE( state.e == 5);
      }
   }

   
} // casual