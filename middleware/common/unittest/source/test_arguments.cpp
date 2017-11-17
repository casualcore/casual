//!
//! casual
//!

#include <gtest/gtest.h>

#include "common/unittest.h"
#include "common/arguments.h"
#include "common/execute.h"


#include <functional>

namespace casual
{

   namespace common
   {

      namespace local
      {
         namespace
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


            template< typename... Args>
            constexpr std::size_t deduce( void (*function)( Args... args))
            {
               return sizeof...( Args);
            }

            void func1() {}
            void func2( int a, int b, int c) {}
            void func3( long a, std::string b, int c, char d, float e) {}
         }

      } // local

      TEST( casual_common_arguments, test_function_argument_deduction)
      {

         EXPECT_TRUE( local::deduce( &local::func1) == 0) << "value: " << local::deduce( &local::func1);
         EXPECT_TRUE( local::deduce( &local::func2) == 3) << "value: " << local::deduce( &local::func2);
         EXPECT_TRUE( local::deduce( &local::func3) == 5) << "value: " << local::deduce( &local::func3);

      }

      TEST( casual_common_arguments, test_bind__vector_string_variable)
      {
         std::vector< std::string> vector_string_value;

         auto dispatch = argument::internal::caller::make( vector_string_value);

         std::vector< std::string> values{ "1", "2", "3"};

         dispatch( values);

         EXPECT_TRUE( vector_string_value == values);
      }


      TEST( casual_common_arguments, test_bind__vector_string_variable_append)
      {
         std::vector< std::string> vector_string_value;

         auto dispatch = argument::internal::caller::make( vector_string_value);

         const std::vector< std::string> values{ "1", "2", "3"};

         dispatch( values);
         EXPECT_TRUE( vector_string_value == values);

         // append again
         dispatch( values);
         EXPECT_TRUE( vector_string_value.size() == values.size() * 2);

      }


      TEST( casual_common_arguments, test_bind)
      {
         local::Conf conf;


         auto dispatch0 = argument::internal::caller::make( &local::Conf::flag, conf);


         dispatch0();

         EXPECT_TRUE( conf.called);


         using namespace std::placeholders;

         auto dispatch1 = argument::internal::caller::make( &local::Conf::setString, conf);

         dispatch1( "234");


         EXPECT_TRUE( conf.string_value == "234");

         auto dispatch2 = argument::internal::caller::make( &local::Conf::setLong, conf);

         dispatch2( 234);

         EXPECT_TRUE( conf.long_value == 234);

         {
            long long_value = 0;
            // bind to value
            auto dispatch3 = argument::internal::caller::make( long_value);
            dispatch3( 666);
            EXPECT_TRUE( long_value == 666);
         }
         
      }


      TEST( casual_common_arguments, directive_deduced_cardinality__member_function_void)
      {

         local::Conf conf;


         Arguments arguments{
            { argument::directive( { "-f", "--foo"}, "some foo stuff", &local::Conf::flag, conf)}
         };

         EXPECT_FALSE( conf.called );

         arguments.parse( { "-f"});

         EXPECT_TRUE( conf.called);

      }

      TEST( casual_common_arguments, directive_deduced_cardinality__member_function_string)
      {

         local::Conf conf;

         Arguments arguments{ { argument::directive( { "-f", "--foo"}, "some foo stuff", &local::Conf::setString, conf)}};

         arguments.parse( { "-f" ,"someValue"});

         EXPECT_TRUE( conf.string_value == "someValue");


      }

      TEST( casual_common_arguments, directive_deduced_cardinality__member_function_long)
      {

         local::Conf conf;

         Arguments arguments{ { argument::directive( { "-f", "--foo"}, "some foo stuff", &local::Conf::setLong, conf)}};

         arguments.parse( { "-f" ,"42"});

         EXPECT_TRUE( conf.long_value == 42);

      }


      TEST( casual_common_arguments, directive_deduced_cardinality__member_function_vector_string)
      {

         local::Conf conf;

         Arguments arguments{ { argument::directive( { "-f", "--foo"}, "some foo stuff", &local::Conf::setVectorString, conf)}};

         arguments.parse( { "-f" ,"42", "666", "777"});

         EXPECT_TRUE( conf.vector_string_value.size() == 3) << " conf.vector_string_value: " << range::make(  conf.vector_string_value);

      }

      TEST( casual_common_arguments, directive_deduced_cardinality__member_function_vector_long)
      {

         local::Conf conf;

         Arguments arguments{ { argument::directive( { "-f", "--foo"}, "some foo stuff", &local::Conf::setVectorLong, conf)}};

         arguments.parse( { "-f" ,"42", "666", "777"});

         EXPECT_TRUE( conf.vector_long_value.size() == 3);

      }

      TEST( casual_common_arguments, directive_any_cardinality__member_function_vector_long__expect_3)
      {

         local::Conf conf;

         Arguments arguments{ {
            argument::directive( argument::cardinality::Any(), { "-f", "--foo"}, "some foo stuff", &local::Conf::setVectorLong, conf)}};

         arguments.parse(  { "-f" ,"42", "666", "777"});

         EXPECT_TRUE( conf.vector_long_value.size() == 3);

      }

      TEST( casual_common_arguments, directive_any_cardinality__member_function_vector_long__expect_0)
      {

         local::Conf conf;

         Arguments arguments{ {
            argument::directive( argument::cardinality::Any(), { "-f", "--foo"}, "some foo stuff", &local::Conf::setVectorLong, conf)}};

         arguments.parse(  { "-f" });

         EXPECT_TRUE( conf.vector_long_value.size() == 0);

      }

      TEST( casual_common_arguments, directive_fixed_3_cardinality__member_function_vector_long__expect_3)
      {

         local::Conf conf;

         Arguments arguments{ {
            argument::directive( argument::cardinality::Fixed< 3>(), { "-f", "--foo"}, "some foo stuff", &local::Conf::setVectorLong, conf)}};

         arguments.parse( { "-f" ,"42", "666", "777"});

         EXPECT_TRUE( conf.vector_long_value.size() == 3);

      }

      TEST( casual_common_arguments, two_directive_deduced_cardinality__member_function_vector_long_and_string)
      {

         local::Conf conf;

         Arguments arguments{ {
            argument::directive( { "-f", "--foo"}, "some foo stuff", &local::Conf::setVectorLong, conf),
            argument::directive( { "-b", "--bar"}, "some bar stuff", &local::Conf::setVectorString, conf)}};

         arguments.parse( { "-b" ,"1", "2", "3", "-f" ,"42", "666", "777"});

         EXPECT_TRUE( conf.vector_string_value.size() == 3);
         EXPECT_TRUE( conf.vector_long_value.size() == 3);

      }


      namespace local
      {
         namespace
         {
            std::vector< std::string> global1;

            void freeFunctionOneToMany( const std::vector< std::string>& value)
            {
               global1 = value;
            }
         }
      } // local

      TEST( casual_common_arguments, function_vector_string)
      {
         Arguments arguments{ {
            argument::directive( { "-f", "--foo"}, "some foo stuff", &local::freeFunctionOneToMany)}};

         arguments.parse( { "-f" ,"1", "2", "3"});

         EXPECT_TRUE( local::global1.size() == 3);

      }

      TEST( casual_common_arguments, vector_double_variable)
      {
         std::vector< double> vector_double_value;

         Arguments arguments{ {
            argument::directive( { "-f", "--foo"}, "some foo stuff", vector_double_value)}};

         arguments.parse( { "-f" ,"1.42", "1.42", "3.33"});


         EXPECT_DOUBLE_EQ( vector_double_value.at( 0), 1.42);
         EXPECT_DOUBLE_EQ( vector_double_value.at( 1), 1.42);
         EXPECT_DOUBLE_EQ( vector_double_value.at( 2), 3.33);
      }

      TEST( casual_common_arguments, lambda)
      {
         std::string value;

         Arguments arguments{ {
            argument::directive( { "-f", "--foo"}, "some foo stuff", [&]( const std::string& v){ value = v;})}};

         arguments.parse( { "-f" ,"bla bla"});

         EXPECT_TRUE( value == "bla bla");
      }

      TEST( casual_common_arguments, lambda_long)
      {
         long value;

         Arguments arguments{ {
            argument::directive( { "-f", "--foo"}, "some foo stuff", [&]( long v){ value = v;})}};

         arguments.parse( { "-f" ,"42"});

         EXPECT_TRUE( value == 42);
      }


      template <typename T>
      struct casual_common_arguments_integrals : public ::testing::Test
      {
         using type = T;

         type random() const
         {
            type result;

            auto range = range::make( reinterpret_cast< char*>( &result), sizeof( type));
            unittest::random::range( range);

            return result;
         }

      };


      typedef ::testing::Types<
            char16_t, 
            char32_t, 
            wchar_t,
            char,
            signed char,
            short,
            int,
            long,
            long long,
            unsigned char,
            unsigned short,
            unsigned int,
            unsigned long,
            unsigned long long
      > pod_types;

      TYPED_TEST_CASE( casual_common_arguments_integrals, pod_types);



      TYPED_TEST( casual_common_arguments_integrals, single_variable_0)
      {
         using type = typename TestFixture::type;

         type variable = 0;

         Arguments arguments{ {
            argument::directive( { "-f"}, "", variable)}};

         arguments.parse( { "-f" ,"0"});

         EXPECT_TRUE( variable == 0);
      }


      TYPED_TEST( casual_common_arguments_integrals, single_variable_max)
      {
         using type = typename TestFixture::type;

         type variable = 0;

         auto max = std::numeric_limits< type>::max();

         Arguments arguments{ {
            argument::directive( { "-f"}, "", variable)}};

         arguments.parse( { "-f" , std::to_string( max)});

         EXPECT_TRUE( variable == max);
      }

      TYPED_TEST( casual_common_arguments_integrals, single_variable_min)
      {
         using type = typename TestFixture::type;

         type variable = 0;

         auto min = std::numeric_limits< type>::min();

         Arguments arguments{ {
            argument::directive( { "-f"}, "", variable)}};

         arguments.parse( { "-f" , std::to_string( min)});

         EXPECT_TRUE( variable == min);
      }

      TYPED_TEST( casual_common_arguments_integrals, single_variable_random)
      {
         using type = typename TestFixture::type;

         type variable = 0;

         auto random = TestFixture::random();

         Arguments arguments{ {
            argument::directive( { "-f"}, "", variable)}};

         arguments.parse( { "-f" , std::to_string( random)});

         EXPECT_TRUE( variable == random);
      }

      TYPED_TEST( casual_common_arguments_integrals, single_variable_random__lambda)
      {
         using type = typename TestFixture::type;

         type variable = 0;

         auto random = TestFixture::random();

         Arguments arguments{ {
            argument::directive( { "-f"}, "", [&]( type v){ variable = v;})
         }};

         arguments.parse( { "-f" , std::to_string( random)});

         EXPECT_TRUE( variable == random);
      }

      TYPED_TEST( casual_common_arguments_integrals, vector_10_variable_random)
      {
         using type = typename TestFixture::type;

         std::vector< type> variable;

         std::vector< type> random( 10);
         for( auto& v : random) { v = TestFixture::random();}

         Arguments arguments{ {
            argument::directive( { "-f"}, "", variable)}};
         
         std::vector< std::string> text{ "-f"};
         range::transform( random, text, []( auto v){ return std::to_string( v);});

         arguments.parse( std::move( text));

         EXPECT_TRUE( variable == random);
      }

      namespace local
      {
         namespace 
         {


            auto bind_to_stdout( std::ostream& out)
            {
               auto origin = std::cout.rdbuf( out.rdbuf());

               return execute::scope( [=](){
                  std::cout.rdbuf( origin);
               });
            }

            std::vector< std::string> consume( std::istream& in)
            {
               std::vector< std::string> result;

               std::string line;
               while( std::getline( in, line))
               {
                  result.push_back( line);
               }
               return result;
            }
         }
      } // local


      TEST( casual_common_arguments_bash_completion, empty__expect_1_lines)
      {
         std::stringstream stream;
         auto guard = local::bind_to_stdout( stream);

         EXPECT_THROW({
            Arguments arguments{{}};
            arguments.parse( { "casual-bash-completion"});      
         }, argument::exception::bash::Completion);

         auto lines = local::consume( stream);

         ASSERT_TRUE( lines.size() == 1) << range::make( lines);
         EXPECT_TRUE( lines.at( 0) == "--help 0 0") << range::make( lines);
      }


      TEST( casual_common_arguments_bash_completion, one_directive_one_many__expect_2_lines)
      {
         std::stringstream stream;
         auto guard = local::bind_to_stdout( stream);

         EXPECT_THROW({
            Arguments arguments{ {
               argument::directive( { "-f", "--foo"}, "some foo stuff", &local::freeFunctionOneToMany)}};
            arguments.parse( { "casual-bash-completion"});      
         }, argument::exception::bash::Completion);

         auto lines = local::consume( stream);

         ASSERT_TRUE( lines.size() == 2) << range::make( lines);
         EXPECT_TRUE( lines.at( 0) == "--foo 1 " + std::to_string( std::numeric_limits< std::size_t>::max())) << range::make( lines);
      }

      TEST( casual_common_arguments_bash_completion, one_directive_any__expect_2_lines)
      {
         std::stringstream stream;
         auto guard = local::bind_to_stdout( stream);

         EXPECT_THROW({
            Arguments arguments{ {
               argument::directive( argument::cardinality::Any{}, { "-f", "--foo"}, "some foo stuff", []( const std::vector< std::string>&){})}};
            arguments.parse( { "casual-bash-completion"});      
         }, argument::exception::bash::Completion);

         auto lines = local::consume( stream);

         ASSERT_TRUE( lines.size() == 2) << range::make( lines);
         EXPECT_TRUE( lines.at( 0) == "--foo 0 " + std::to_string( std::numeric_limits< std::size_t>::max())) << range::make( lines);
      }

      TEST( casual_common_arguments_bash_completion, one_directive_cardinality_3__expect_2_lines)
      {
         std::stringstream stream;
         auto guard = local::bind_to_stdout( stream);

         EXPECT_THROW({
            Arguments arguments{ {
               argument::directive( argument::cardinality::Fixed< 3>{}, { "-f", "--foo"}, "some foo stuff", []( const std::vector< std::string>&){})}};
            arguments.parse( { "casual-bash-completion"});
            
         }, argument::exception::bash::Completion);

         auto lines = local::consume( stream);

         ASSERT_TRUE( lines.size() == 2) << range::make( lines);
         EXPECT_TRUE( lines.at( 0) == "--foo 3 3" ) << range::make( lines);
      }
   }
}



