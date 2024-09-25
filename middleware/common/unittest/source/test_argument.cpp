//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/argument.h"
#include "common/argument/cardinality.h"
#include "common/code/casual.h"
#include "common/execute.h"


namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            auto invocable_0 = [](){};
            auto invocable_1 = []( int v1){};
            //auto invocable_2 = []( int v1, int v2){};
            auto invocable_any = []( std::vector< int> v1){};

            std::string newline() { return "\n";}
         } // <unnamed>
      } // local


      TEST( common_argument_option, option_copy_ctor)
      {
         unittest::Trace trace;

         argument::Option option{ local::invocable_0, { "-a"}, "test"};

         argument::Option copy = option;

         EXPECT_TRUE( option.description() == copy.description());
      }


      TEST( common_argument_option, default_ctor__expect_no_keys)
      {
         unittest::Trace trace;

         argument::Option option{ local::invocable_0, { "-a"}, "test"};

         EXPECT_FALSE( option.has( "key"));
      }

      TEST( common_argument_option, key__expect_has_key)
      {
         unittest::Trace trace;

         argument::Option option{ local::invocable_0, { "--key"}, "test"};

         EXPECT_TRUE( option.has( "--key"));
      }

      TEST( common_argument_option_one_many, value_vector)
      {
         unittest::Trace trace;

         std::vector< int> values;

         argument::Option option{ argument::option::one::many( values), { "-a"}, "test"};

         EXPECT_TRUE( option.representation().invocable.cardinality() == argument::cardinality::one_many{}) << "option.representation(): " << option.representation();

         std::vector< std::string_view> arguments{ "1", "2", "3"};

         option.assign( "-a", range::make( arguments));
         option.invoke();

         EXPECT_TRUE(( values == std::vector< int>{ 1, 2, 3}));
      }



      TEST( common_argument_group, expect__has_key)
      {
         unittest::Trace trace;

         argument::Group group{ local::invocable_0, { "group"}, "test"};

         EXPECT_TRUE( group.has( "group"));
      }

      TEST( common_argument_group, option_1__expect__group_has_key__not_option)
      {
         unittest::Trace trace;

         argument::Group group{ local::invocable_0, { "group"}, "test", argument::Option{ local::invocable_0, { "--key"}, "test"}};

         EXPECT_TRUE( group.has( "group"));
         EXPECT_FALSE( group.has( "--key"));
      }

      TEST( common_argument_group, option_1__assign__expect__group_has_key_and_option)
      {
         unittest::Trace trace;

         argument::Group group{ local::invocable_0, { "group"}, "test", argument::Option{ local::invocable_0, { "--key"}, "test"}};

         EXPECT_TRUE( group.has( "group"));
         EXPECT_TRUE( ! group.has( "--key"));

         group.assign( "group", {});

         EXPECT_TRUE( group.has( "--key"));
      }

      TEST( common_argument_group, option_2__expect__has_key)
      {
         unittest::Trace trace;

         argument::Group group{ local::invocable_0, { "group"}, "test", 
            argument::Option{ local::invocable_0, { "--a"}, "a"},
            argument::Option{ local::invocable_0, { "--b"}, "b"}};

         EXPECT_TRUE( group.has( "group"));
         EXPECT_TRUE( ! group.has( "--a"));
         EXPECT_TRUE( ! group.has( "--b"));
         
         group.assign( "group", {});

         EXPECT_TRUE( group.has( "--a"));
         EXPECT_TRUE( group.has( "--b"));
      }


      TEST( common_argument_group, value_semantics)
      {
         unittest::Trace trace;

         auto g1 = argument::Group{ local::invocable_0, { "group"}, "test", 
                     argument::Option{ local::invocable_0, { "--a"}, "a"}};

         auto g2 = g1;

         EXPECT_TRUE( g1.has( "group"));
         EXPECT_TRUE( ! g1.has( "--a"));
         EXPECT_TRUE( g2.has( "group"));
         EXPECT_TRUE( ! g2.has( "--a"));
         
         g1.assign( "group", {});

         EXPECT_TRUE( g1.has( "--a"));

         // g2 should be unchanged
         EXPECT_TRUE( g2.has( "group"));
         EXPECT_TRUE( ! g2.has( "--a"));
      }

      TEST( common_argument_group, nested_groups)
      {
         unittest::Trace trace;

         argument::Group g1{ local::invocable_0, { "g1"}, "g1", 
            argument::Group{ local::invocable_0, { "g2"}, "g2",
               argument::Option{ local::invocable_0, { "--a"}, "a"}}};

         EXPECT_TRUE( g1.has( "g1"));
         g1.assign( "g1", {});

         EXPECT_TRUE( g1.has( "g2"));
         g1.assign( "g2", {});

         EXPECT_TRUE( g1.has( "--a"));
      }

      TEST( common_argument_option, default_cardinality__expect_0_1)
      {
         unittest::Trace trace;

         argument::Option o1{ local::invocable_0, { "--a"}, "a"};

         EXPECT_TRUE( o1.representation().cardinality == argument::cardinality::zero_one{});
      }

      TEST( common_argument_option, fixed_cardinality__expect_3_3)
      {
         unittest::Trace trace;

         auto o1 = argument::Option{ local::invocable_0, { "--a"}, "a"}( argument::cardinality::fixed< 3>{});

         EXPECT_TRUE( o1.representation().cardinality == argument::cardinality::fixed< 3>{});
      }

      TEST( common_argument_invoke, tuple_1__expect_cardinality_1)
      {
         unittest::Trace trace;

         int a = 0;
         auto invoke = argument::detail::invoke::create( std::tie( a));

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::fixed< 1>{});
      }

      TEST( common_argument_invoke, tuple_2__expect_cardinality_2)
      {
         unittest::Trace trace;

         int a = 0;
         std::string b;
         auto invoke = argument::detail::invoke::create( std::tie( a, b));

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::fixed< 2>{}) << "cardinality: " << invoke.cardinality();
      }


      TEST( common_argument_invoke, tuple_2__last_is_vector__expect_1_many)
      {
         unittest::Trace trace;

         int a = 0;
         std::vector< int> b;
         auto invoke = argument::detail::invoke::create( std::tie( a, b));

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::one_many{}) << "cardinality: " << invoke.cardinality();
      }


      TEST( common_argument_invoke, lambda)
      {
         unittest::Trace trace;

         auto invoke = argument::detail::invoke::create( []( int a, const std::string& b){
            EXPECT_TRUE( a == 42);
            EXPECT_TRUE( b == "poop");
         });

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::fixed< 2>{});
      }


      TEST( common_argument_invoke, value_cardinality_any)
      {
         unittest::Trace trace;

         auto invoke = argument::detail::invoke::create( []( std::vector< int> values){
         });

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::any{});
      }

      TEST( common_argument_invoke, value_cardinality_nested_tuple_2__expect_cardinality_3)
      {
         unittest::Trace trace;

         std::tuple< int, std::string> tuple;
         int value;

         auto invoke = argument::detail::invoke::create( std::tie( value, tuple));

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::fixed< 3>{});
      }

      TEST( common_argument_invoke, value_cardinality__optional__expect_zero_one)
      {
         unittest::Trace trace;

         std::optional< int> value;

         auto invoke = argument::detail::invoke::create( std::tie( value));

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::zero_one{});
         EXPECT_TRUE( invoke.cardinality().step() == 1) << "cardinality: " << invoke.cardinality();
      }

      TEST( common_argument_invoke, value_cardinality__optional_tuple_3__expect_max_3__step_3)
      {
         unittest::Trace trace;

         std::optional< std::tuple< int, int, int>> value;

         auto invoke = argument::detail::invoke::create( std::tie( value));

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::max< 3>{});
         EXPECT_TRUE( invoke.cardinality().step() == 3) << "cardinality: " << invoke.cardinality();
      }

      TEST( common_argument_invoke, value_cardinality__value_1_optional_tuple_3__expect_range_1_4__step_3)
      {
         unittest::Trace trace;
         
         int a;
         std::optional< std::tuple< int, int, int>> b;

         auto invoke = argument::detail::invoke::create( std::tie( a, b));
         
         EXPECT_TRUE(( invoke.cardinality() == argument::cardinality::range< 1, 4, 3>{})) << "cardinality: " << invoke.cardinality();
      }

      TEST( common_argument_invoke, value_cardinality_vector_with_tuple___expect_cardinality_Any)
      {
         unittest::Trace trace;

         std::vector< std::tuple< int, int>> value;

         auto invoke = argument::detail::invoke::create( std::tie( value));

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::any{}) << "cardinality: " << invoke.cardinality();

      }


      TEST( common_argument_invoke, value_cardinality_tuple_2_and__vector_with_tuple___expect_cardinality__min_2)
      {
         unittest::Trace trace;

         std::tuple< int, int> a;
         std::vector< std::tuple< int, int>> b;
         

         auto invoke = argument::detail::invoke::create( std::tie( a, b));
         

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::min< 2>{}) << "cardinality: " << invoke.cardinality();
         EXPECT_TRUE( invoke.cardinality().step() == 2) << "cardinality: " << invoke.cardinality();
      }

      TEST( common_argument_invoke, value_cardinality__vector_with_nested_tuple___expect_cardinality__any_step_3)
      {
         unittest::Trace trace;

         std::vector< std::tuple< int, std::tuple< long, float>>> a;

         auto invoke = argument::detail::invoke::create( std::tie( a));
         

         EXPECT_TRUE( invoke.cardinality() == argument::cardinality::any{}) << "cardinality: " << invoke.cardinality();
         EXPECT_TRUE( invoke.cardinality().step() == 3) << "cardinality: " << invoke.cardinality();
      }


      TEST( common_argument_parse, empty_parser)
      {
         unittest::Trace trace;

         argument::Parse parse{ "description"};

         EXPECT_NO_THROW({
            parse( std::vector< std::string>{});
         });
      }

      TEST( common_argument_parse, option_1)
      {
         unittest::Trace trace;

         int value;
         argument::Parse parse{ "description", 
            argument::Option{ std::tie( value), { "-a"}, "description"}};
          
         EXPECT_NO_THROW({
            parse( { "-a", "42"});
         });
         EXPECT_TRUE( value == 42);
      }

      TEST( common_argument_parse, option_3)
      {
         unittest::Trace trace;

         bool v_bool = false;
         argument::Parse parse{ "description", 
            argument::Option{ std::tie( v_bool), { "-a"}, "description"},
            argument::Option{ local::invocable_0, { "-b"}, "description"}
         };
          
         EXPECT_NO_THROW({
            parse( { "-a", "1", "-b"});
         });
         EXPECT_TRUE( v_bool == true);
      }

      TEST( common_argument_parse, option_any_cardinality__vector_value___4_arguments___expect_accumulated_values)
      {
         unittest::Trace trace;

         std::vector< std::string> values;
         argument::Parse parse{ "description", 
            argument::Option{ std::tie( values), { "-a"}, ""}( argument::cardinality::any{}),
         };
          
         EXPECT_NO_THROW({
            parse( { "-a", "1", "2", "-a", "3", "4", "-a", "5", "6", "-a", "7", "8"});
         });

         EXPECT_TRUE(( values == std::vector< std::string>{ "1", "2", "3", "4", "5", "6", "7", "8", }));
      }

      TEST( common_argument_parse, option_any_cardinality__vector_value__argument_has_ws___expect_accumulated_values)
      {
         unittest::Trace trace;

         std::vector< std::string> values;
         argument::Parse parse{ "description", 
            argument::Option{ std::tie( values), { "-a"}, ""}( argument::cardinality::any{}),
         };
          
         EXPECT_NO_THROW({
            parse( { "-a", "1 2", "-a", "3 4"});
         });

         EXPECT_TRUE(( values == std::vector< std::string>{ "1 2", "3 4"})) << trace.compose( "values: ", values);
      }

      TEST( common_argument_parse, option_deprecated_keys)
      {
         unittest::Trace trace;

         int value{};
         argument::Parse parse{ "description", 
            argument::Option{ std::tie( value), argument::option::keys( { "-a"}, { "--deprecated"}), "description"}};
          
         EXPECT_NO_THROW({
            parse( { "--deprecated", "42"});
         });
         EXPECT_TRUE( value == 42);
      }

      TEST( common_argument_option, lambda_tuple_string_tuple_int__string)
      {
         unittest::Trace trace;

         auto lambda = []( const std::tuple< std::string, std::tuple< std::string, int>>& v){
         };

         argument::Parse parse{ "description", 
            argument::Option{ lambda, { "-a"}, ""}
         };

         EXPECT_NO_THROW({
            parse( { "-a", "string", "string2", "666"});
         });
      }

      TEST( common_argument_option, lambda_tuple_string_int__tuple_int__string)
      {
         unittest::Trace trace;

         auto lambda = []( const std::tuple< std::string, short>& v1, std::tuple< std::string, long>& v2){
         };

         argument::Parse parse{ "description", 
            argument::Option{ lambda, { "-a"}, ""}
         };

         EXPECT_NO_THROW({
            parse( { "-a", "string", "42", "string2", "666"});
         });
      }


      TEST( common_argument_option, lambda_string_int_string_long)
      {
         unittest::Trace trace;

         auto lambda = []( const std::string&, int, const std::string&, long ){
         };

         argument::Parse parse{ "description", 
            argument::Option{ lambda, { "-a"}, ""}
         };

         EXPECT_NO_THROW({
            parse( { "-a", "string", "42", "string2", "666"});
         });
      }
      
      TEST( common_argument_option, lambda_tuple_string_int_string_long)
      {
         unittest::Trace trace;

         auto lambda = []( const std::tuple< std::string, int, std::string, long>& value ){
         };

         argument::Parse parse{ "description", 
            argument::Option{ lambda, { "-a"}, ""}
         };

         EXPECT_NO_THROW({
            parse( { "-a", "string", "42", "string2", "666"});
         });
      }


      TEST( common_argument_option, lambda_tuple_string_int)
      {
         unittest::Trace trace;

         auto lambda = []( const std::tuple< std::string, int>& value){
         };

         argument::Parse parse{ "description", 
            argument::Option{ lambda, { "-a"}, ""}
         };

         EXPECT_NO_THROW({
            parse( { "-a", "string", "42"});
         });
      }

      TEST( common_argument_option, lambda__vector_with_tuple_string_int___expect_parse)
      {
         unittest::Trace trace;

         auto lambda = []( const std::vector< std::tuple< std::string, int>>& values){
         };

         argument::Parse parse{ "description", 
            argument::Option{ lambda, { "-a"}, ""}
         };

         EXPECT_NO_THROW({
            parse( { "-a", "string", "42"});
         });
      }


      TEST( common_argument_option, lambda_tuple_string_int__vector_with_tuple_string_int___expect_parse)
      {
         unittest::Trace trace;

         auto lambda = []( const std::tuple< std::string, int>& value, const std::vector< std::tuple< std::string, int>>& values){
         };

         argument::Parse parse{ "description", 
            argument::Option{ lambda, { "-a"}, ""}
         };

         EXPECT_NO_THROW({
            parse( { "-a", "string", "42"});
         });
      }


      TEST( common_argument_parse, optional_tuple_3___3_values__expect_parse)
      {
         unittest::Trace trace;

         std::optional< std::tuple< int, int, int>> value;
         argument::Parse parse{ "description", 
            argument::Option{ std::tie( value), { "-a"}, "description"}};
          
         EXPECT_NO_THROW({
            parse( { "-a", "42", "43", "44"});
         });
         ASSERT_TRUE( value.has_value());
         EXPECT_TRUE( std::get< 0>( value.value()) == 42);
         EXPECT_TRUE( std::get< 1>( value.value()) == 43);
         EXPECT_TRUE( std::get< 2>( value.value()) == 44);
      }

      TEST( common_argument_parse, optional_tuple_3___0_values__expect_parse)
      {
         unittest::Trace trace;

         std::optional< std::tuple< int, int, int>> value;
         argument::Parse parse{ "description", 
            argument::Option{ std::tie( value), { "-a"}, "description"}};
          
         EXPECT_NO_THROW({
            parse( { "-a"});
         });
         ASSERT_TRUE( ! value.has_value());
      }

      TEST( common_argument_parse, optional_tuple_3___2_values__expect_throw)
      {
         unittest::Trace trace;

         std::optional< std::tuple< int, int, int>> value;
         argument::Parse parse{ "description", 
            argument::Option{ std::tie( value), { "-a"}, "description"}};

         EXPECT_CODE({
            parse( { "-a", "42", "43"});
         }, code::casual::invalid_argument);
      }

      TEST( common_argument_parse, group_with_values)
      {
         unittest::Trace trace;

         int value;
         argument::Parse parse{ "description", 
            argument::Group{ std::tie( value), { "-a"}, "description"}};
          
         EXPECT_NO_THROW({
            parse( { "-a", "42"});
         });
         EXPECT_TRUE( value == 42);
      }

      TEST( common_argument_parse, option_a__group_g_with_option_a__expect_g_a__invoke_group_a)
      {
         unittest::Trace trace;

         int a = 0;
         int g_a = 0;
         argument::Parse parse{ "description", 
            argument::Option( std::tie( a), {"-a"}, ""),  
            argument::Group{ local::invocable_0, { "-g"}, "",
               argument::Option( std::tie( g_a), {"-a"}, "")
            }
         };
          
         EXPECT_NO_THROW({
            parse( { "-g", "-a", "42"});
         });
         EXPECT_TRUE( a == 0);
         EXPECT_TRUE( g_a == 42);
      }

      TEST( common_argument_parse, option_a__group_g_with_option_a__expect_a__invoke_a)
      {
         unittest::Trace trace;

         int a = 0;
         int g_a = 0;
         argument::Parse parse{ "description", 
            argument::Option( std::tie( a), {"-a"}, ""),  
            argument::Group{ local::invocable_0, { "-g"}, "",
               argument::Option( std::tie( g_a), {"-a"}, "")
            }
         };
          
         EXPECT_NO_THROW({
            parse( { "-a", "42"});
         });
         EXPECT_TRUE( a == 42);
         EXPECT_TRUE( g_a == 0);
      }

      TEST( common_argument_parse, group_with_1_option)
      {
         unittest::Trace trace;

         bool g = false;
         int value;
         argument::Parse parse{ "description", 
            argument::Group{ [&g](){ g = true;}, { "-g"}, "description",
               argument::Option( std::tie( value), {"-a"}, "description")   
            }
         };
          
         EXPECT_NO_THROW({
            parse( { "-g", "-a", "42"});
         });
         EXPECT_TRUE( g);
         EXPECT_TRUE( value == 42);
      }

      TEST( common_argument_parse, group_invoked_callback__expect_options_to_be_set)
      {
         unittest::Trace trace;

         bool g = false;
         int value{};

         auto invoked = [&]()
         {
            EXPECT_TRUE( g);
            EXPECT_TRUE( value == 42);
         };

         argument::Parse parse{ "description", 
            argument::Group{ invoked, [&g](){ g = true;}, { "-g"}, "description",
               argument::Option( std::tie( value), {"-a"}, "description")   
            }
         };
          
         EXPECT_NO_THROW({
            parse( { "-g", "-a", "42"});
         });
      }


      TEST( common_argument_parse, group_with_1_invoke_and_callback__expect_invoke_to_be_invoked_last)
      {
         unittest::Trace trace;

         bool a = false;

         auto invoked = [&]()
         {
            EXPECT_TRUE( a);
         };

         auto callback = [&]()
         {
            EXPECT_FALSE( a);
         };

         argument::Parse parse{ "description", 
            argument::Group{ invoked, callback, { "-g"}, "description",
               argument::Option( [ &a](){ a = true;}, {"-a"}, "description")   
            }
         };
          
         EXPECT_NO_THROW({
            parse( { "-g", "-a"});
         });

      }

      TEST( common_argument_parse, option_1__values_0__expect_throw)
      {
         unittest::Trace trace;

         int value;
         argument::Parse parse{ "description", 
            argument::Option{ std::tie( value), { "-a"}, "description"}};
          
         EXPECT_CODE({
            parse( { "-a"});
         }, code::casual::invalid_argument);
      }

      TEST( common_argument_parse, option_1__values_2__expect_throw)
      {
         unittest::Trace trace;

         int value;
         argument::Parse parse{ "description", 
            argument::Option{ std::tie( value), { "-a"}, "description"}};
          
         EXPECT_CODE({
            parse( { "-a", "42", "42"});
         }, code::casual::invalid_argument);
      }

      TEST( common_argument_parse, option_tuple_var_2__values_1__expect_throw)
      {
         unittest::Trace trace;

         int a;
         long b;

         argument::Parse parse{ "description", 
            argument::Option{ std::tie( a, b), { "-a"}, "description"}};
          
         EXPECT_CODE({
            parse( { "-a", "42"});
         }, code::casual::invalid_argument);
      }
/*
      TEST( common_argument_parse, nested_option__2_levels___expect_values_to_be_set)
      {
         unittest::Trace trace;

         int a;
         long b;
         long c;

         argument::Parse parse{ "description", 
            argument::Group{ local::invocable_0, { "some-group"}, "",
               argument::Option{ std::tie( a, b), { "-a",  "--alfa"}, ""},
               argument::Group{ local::invocable_0, { "some-other-group"}, "",
                  argument::Option{ std::tie( c), { "-b", "--beta"}, ""} 
               }
            }
         };

         EXPECT_NO_THROW({
            parse( { 
               "some-group",
               "--alfa",
               "42",
               "45",
               "some-other-group",
               "--beta",
               "666"
            });
         });

         EXPECT_TRUE( a == 42);
         EXPECT_TRUE( b == 45);
         EXPECT_TRUE( c == 666);
      }
      */

      TEST( common_argument_parse, builtin_help__expect_throw)
      {
         unittest::Trace trace;

         int a;
         long b;

         argument::Parse parse{ "description", 
            argument::Group{ local::invocable_0, { "some-group"}, "group\ndescription\nblabla bla",
               argument::Option{ std::tie( a, b), { "--alfa", "-a"}, "description"},
               argument::Group{ local::invocable_1, { "some-other-group"}, "sub-group...",
                  argument::Option{ local::invocable_any, { "--beta", "-b"}, "beta stuff..."} 
               }
            }
         };
          
         EXPECT_NO_THROW({
            parse( { "--help"});
         });
      }

      TEST( common_argument_parse, builtin_help__complex_cardinality_output)
      {
         unittest::Trace trace;

         std::tuple< int, std::string, std::vector< std::tuple< int, int>>> a;

         argument::Parse parse{ "description", 
               argument::Option{ a, { "--alfa", "-a"}, "description"},
         };
          
         EXPECT_NO_THROW({
            parse( { "--help"});
         });
      }

      TEST( common_argument_parse, builtin_help__completer_output)
      {
         unittest::Trace trace;

         auto completer = []( auto values, bool){
            return std::vector< std::string>{ "a", "b", "c", "d"};
         };
         std::string a;

         argument::Parse parse{ "description", 
               argument::Option{ std::tie( a), completer, { "--alfa", "-a"}, "description"},
         };
          
         EXPECT_NO_THROW({
            parse( { "--help"});
         });
      }



      TEST( common_argument_parse, complex__)
      {
         unittest::Trace trace;

         int a;
         long b;;

         argument::Parse parse{ "description", 
            argument::Group{ local::invocable_0, { "some-group"}, "group\ndescription\nblabla bla",
               argument::Option{ std::tie( a, b), { "--alfa", "-a"}, "description"},
               argument::Group{ local::invocable_1, { "some-other-group"}, "sub-group...",
                  argument::Option{ local::invocable_0, { "--beta", "-b"}, "beta stuff..."} 
               }
            }
         };
          
         EXPECT_NO_THROW({
            parse( { argument::reserved::name::help()});
         });
      }



      TEST( common_argument_completion, no_option__expect_0_completion)
      {
         unittest::Trace trace;

         argument::Parse parse{ "description"};

         std::ostringstream out;
         auto capture = unittest::capture::standard::out( out);

         EXPECT_NO_THROW({
            parse( { argument::reserved::name::completion()});
         });

         EXPECT_TRUE( out.str().empty());
      }

      TEST( common_argument_completion, one_option__expect_1_completion)
      {
         unittest::Trace trace;

         argument::Parse parse{ "description",
            argument::Option{ local::invocable_1, { "-a", "--alfa"}, "description"},
         };

         std::ostringstream out;
         auto capture = unittest::capture::standard::out( out);

         EXPECT_NO_THROW({
            parse( { argument::reserved::name::completion()});
         });

         EXPECT_TRUE( out.str() == "--alfa\n") << " out.str(): " <<  out.str();
      }

      TEST( common_argument_completion, one_option__parse_one__expect_value_completion)
      {
         unittest::Trace trace;

         argument::Parse parse{ "description",
            argument::Option{ local::invocable_1, { "-a", "--alfa"}, "description"},
         };

         std::ostringstream out;
         auto capture = unittest::capture::standard::out( out);

         EXPECT_NO_THROW({
            parse( { 
               argument::reserved::name::completion(),
               "--alfa"
            });
         });

         EXPECT_TRUE( out.str() == argument::reserved::name::suggestions::value() + local::newline()) << " out.str(): " <<  out.str();
      }

      TEST( common_argument_completion, nested_option__1_level__option_needs_value___expect_value_completion)
      {
         unittest::Trace trace;

         int a;
         long b;

         argument::Parse parse{ "description", 
            argument::Group{ local::invocable_0, { "some-group"}, "",
               argument::Option{ std::tie( a, b), { "-a",  "--alfa"}, ""},
               argument::Option{ local::invocable_1, { "-b", "--beta"}, ""} 
            }
         };

         std::ostringstream out;
         auto capture = unittest::capture::standard::out( out);

         EXPECT_NO_THROW({
            parse( { 
               argument::reserved::name::completion(),
               "some-group",
               "--alfa",
               "42",
               "45",
               "--beta",
            });
         });

         EXPECT_TRUE( out.str() == argument::reserved::name::suggestions::value() + local::newline()) << " out.str(): " <<  out.str();
      }

      TEST( common_argument_completion, nested_option__1_level__option_has_optional_value___expect_option_and_value_completion)
      {
         unittest::Trace trace;

         int a;
         long b;
         std::optional< long> c;

         argument::Parse parse{ "description", 
            argument::Group{ local::invocable_0, { "some-group"}, "",
               argument::Option{ std::tie( a, b), { "-a",  "--alfa"}, ""},
               argument::Option{ std::tie( c), { "-b", "--beta"}, ""} 
            }
         };

         std::ostringstream out;
         auto capture = unittest::capture::standard::out( out);

         parse( { 
            argument::reserved::name::completion(),
            "some-group",
            "--beta",
         });


         auto expected = argument::reserved::name::suggestions::value() + local::newline() + "--alfa" + local::newline();

         EXPECT_TRUE( out.str() == expected) << "expected: " << expected << "\nout.str(): " <<  out.str();
      }

      TEST( common_argument_completion, nested_option__3_groups___expect_option_and_value_completion)
      {
         unittest::Trace trace;

         std::optional< long> a;
         long b;
         short c;

         argument::Parse parse{ "", 
            argument::Group{ local::invocable_0, { "a-group"}, "",
               argument::Option{ std::tie( a), { "-a"}, ""},
            },
            argument::Group{ local::invocable_0, { "b-group"}, "",
               argument::Option{ std::tie( b), { "-b"}, ""} 
            },
            argument::Group{ local::invocable_0, { "c-group"}, "",
               argument::Option{ std::tie( c), { "-c"}, ""} 
            }
         };

         std::ostringstream out;
         auto capture = unittest::capture::standard::out( out);

         parse( { 
            argument::reserved::name::completion(),
            "a-group",
            "-a",
            "42",
            "b-group"
         });


         auto expected = R"(-b
c-group
)";

         EXPECT_TRUE( out.str() == expected) << " out.str(): " <<  out.str() << "\nexpected: " << expected;
      }

      TEST( common_argument_completion, completer)
      {
         unittest::Trace trace;

         auto completer = []( auto arguments, bool){
            return std::vector< std::string>{ "a", "b", "c"};
         };

         argument::Parse parse{ "description",
            argument::Option{ local::invocable_1, completer, { "-a", "--alfa"}, "description"},
         };

         std::ostringstream out;
         auto capture = unittest::capture::standard::out( out);

         parse( { 
            argument::reserved::name::completion(),
            "--alfa"
         });

         EXPECT_TRUE( out.str() == "a\nb\nc\n") << " out.str(): " <<  out.str();
      }
   } // common
} // casual
