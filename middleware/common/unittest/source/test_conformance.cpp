//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/unittest.h"
#include "common/unittest/file.h"

#include "common/stream.h"
#include "common/algorithm.h"
#include "common/traits.h"
#include "common/signal.h"
#include "common/code/system.h"
#include "common/strong/type.h"
#include "common/environment.h"
#include "common/file.h"

#include <type_traits>



#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <csignal>


//
// Just a place to test C++ and POSIX conformance, or rather, casual's view on conformance
//


namespace casual
{

   namespace common
   {



      // static test of concepts

      static_assert( concepts::same_as< char, char, char, char, char>, "concepts::same_as does not work...");

      static_assert( concepts::string::like< decltype( "some string")>, "concepts::string::like does not work...");

      static_assert( concepts::string::like< char const (&)[7]>, "concepts::string::like does not work...");
      static_assert( concepts::string::like< char[ 20]>, "concepts::string::like does not work...");

      static_assert( ! concepts::string::like< char* [ 20]>, "concepts::string::like does not work...");


      static_assert( concepts::range< char[ 20]>, "traits::is::iterable does not work...");

      static_assert( ! concepts::any_of< char, unsigned char, signed char>, "concepts::any_of does not work...");
      static_assert( concepts::any_of< char, unsigned char, signed char, char>, "concepts::any_of does not work...");

      static_assert( concepts::tuple::like< std::pair< int, long>>, "concepts::tuple::like does not work...");

      static_assert( concepts::range_with_value< std::vector< int>, int>, "concepts not work...");
      static_assert( ! concepts::range_with_value< std::vector< int>, long>, "concepts not work...");


      static_assert( concepts::binary::like< std::vector< char>>, "concepts not work...");
      static_assert( ! concepts::binary::like< std::vector< int>>, "concepts not work...");
      static_assert( concepts::binary::iterator< std::vector< char>::iterator>, "concepts not work...");
      static_assert( ! concepts::binary::iterator< std::vector< int>::iterator>, "concepts not work...");

      namespace detail 
      {
         template< typename T>
         struct Specialization;

         template< concepts::range T>
         struct Specialization< T>
         {
            constexpr static std::string_view type() noexcept { return "range";}
         };

         template< std::integral T>
         struct Specialization< T>
         {
            constexpr static std::string_view type() noexcept { return "integral";}
         };

         template< typename T>
         requires std::floating_point< T>
         struct Specialization< T> 
         {
            constexpr static std::string_view type() noexcept { return "floating_point";}
         };

         template< concepts::range T>
         requires concepts::string::like< T>
         struct Specialization< T>
         {
            constexpr static std::string_view type() noexcept { return "string_like";}
         };
         
      } // detail

      static_assert( detail::Specialization< std::vector< int>>::type() == "range");
      static_assert( detail::Specialization< int>::type() == "integral");
      static_assert( detail::Specialization< double>::type() == "floating_point");
      static_assert( detail::Specialization< std::string>::type() == "string_like");


      template< int... values>
      constexpr auto size_of_parameter_pack()
      {
         return sizeof...( values);
      }

      static_assert( size_of_parameter_pack<>() == 0);
      static_assert( size_of_parameter_pack< 1, 2, 3>() == 3);

      namespace detail
      {
         template< typename F>
         auto switch_fallthrough( int value, F&& function)
         {
            switch( value)
            {
               case 1: function( value); // case is doing stuff
               [[fallthrough]];          // hence, need explict fallthrough
               case 2:                   // case is not doing stuff, no fallthrough needed.
               case 3: return 3;
            }
            return 0;
         }

         template< typename to_type, typename from_type>
         auto cast_and_compare( from_type value)
         {
            to_type casted = static_cast< to_type>( value);
            return static_cast< from_type>( casted) == value;
         }
         
      } // detail

      TEST( common_conformance, static_cast_by_value)
      {
         EXPECT_TRUE( detail::cast_and_compare< char>( std::uint8_t{ 42}));
         EXPECT_TRUE( detail::cast_and_compare< char>( std::int8_t{ 42}));
         EXPECT_TRUE( detail::cast_and_compare< short>( std::int16_t{ 42}));
         EXPECT_TRUE( detail::cast_and_compare< short>( std::uint16_t{ 42}));
         EXPECT_TRUE( detail::cast_and_compare< int>( long{ 42}));
      }

      template< typename V, std::predicate< const V> P>
      bool test_predicate( const V& value, P predicate)
      {
         return predicate( value);
      }

      TEST( common_conformance, concepts)
      {
         EXPECT_TRUE( test_predicate( 21, []( auto value){ return value == 21;}));

      }

      namespace detail
      {
         template< concepts::range R>
         auto sort( R&& range)  
            -> decltype( void( std::sort( std::begin( range), std::end( range))), std::forward< R>( range))
         {
            std::sort( std::begin( range), std::end( range));
            return std::forward< R>( range);
         }

         struct A
         {
            const char* a;
            long b;
            short c;

            friend auto operator<=>( const A&, const A&) = default;
         };

      } // detail

      TEST( common_conformance, range)
      {
         auto as = std::vector< detail::A>{ { "1", 1, 2}, { "2", 2, 3}};

         auto result = as | std::views::filter( []( auto& v){ return v.b == 2;}) | std::views::transform( []( auto& v){ return v.c;});

         EXPECT_TRUE( range::front( result) == 3);

      }


      TEST( common_conformance, fallthrough)
      {
         int value{};
         auto function = [&value]( auto v){ value = v;};

         EXPECT_TRUE( detail::switch_fallthrough( 1, function) == 3);
         EXPECT_TRUE( value == 1);
      }
      
      TEST( common_conformance, filesystem_path__environment_expansion)
      {
         auto path = std::filesystem::path{ "${FOO}/${BAR}"};
         auto range = range::make( path);
         EXPECT_TRUE( range.size() == 2);
      }

      TEST( common_conformance, struct_with_pod_attributes__is_pod)
      {
         struct POD
         {
            int int_value;
         };

         EXPECT_TRUE( std::is_standard_layout< POD>::value);
      }

      TEST( common_conformance, struct_with_member_function__is_pod)
      {
         struct POD
         {
            int func1() { return 42;}
         };

         EXPECT_TRUE( std::is_standard_layout< POD>::value);
      }


      TEST( common_conformance, is_floating_point__is_signed)
      {

         EXPECT_TRUE( std::is_signed< float>::value);
         EXPECT_TRUE( std::is_floating_point< float>::value);

         EXPECT_TRUE( std::is_signed< double>::value);
         EXPECT_TRUE( std::is_floating_point< double>::value);

      }





      struct some_functor
      {
         void operator () ( const double& value)  {}

         int test;
      };


      TEST( common_conformance, get_functor_argument_type)
      {

         EXPECT_TRUE( traits::function< some_functor>::arguments() == 1);

         using argument_type = typename traits::function< some_functor>::argument< 0>::type;

         auto is_same = std::is_same< const double&, argument_type>::value;
         EXPECT_TRUE( is_same);
      }

      TEST( common_conformance, get_function_argument_type)
      {
         using function_1 = std::function< void( double&)>;

         EXPECT_TRUE( traits::function< function_1>::arguments() == 1);

         using argument_type = typename traits::function< function_1>::argument< 0>::type;

         auto is_same = std::is_same< double&, argument_type>::value;
         EXPECT_TRUE( is_same);
      }

      TEST( common_conformance, get_function_two_arguments_types)
      {
         using function_1 = std::function< void( double&, long&)>;

         EXPECT_TRUE( traits::function< function_1>::arguments() == 2);

         using argument1_type = typename traits::function< function_1>::argument< 0>::type;
         using argument2_type = typename traits::function< function_1>::argument< 1>::type;

         EXPECT_TRUE(( std::is_same_v< double&, argument1_type>));
         EXPECT_TRUE(( std::is_same_v< long&, argument2_type>));
      }

      long some_function( const std::string& value) { return 1;}

      TEST( common_conformance, get_free_function_argument_type)
      {
         using traits_type = traits::function< decltype( &some_function)>;

         EXPECT_TRUE( traits_type::arguments() == 1);

         {
            using argument_type = typename traits_type::argument< 0>::type;

            auto is_same = std::is_same< const std::string&, argument_type>::value;
            EXPECT_TRUE( is_same);
         }

         {
            using result_type = typename traits_type::result_type;

            auto is_same = std::is_same< long, result_type>::value;
            EXPECT_TRUE( is_same);
         }
      }


      TEST( common_conformance, search)
      {
         std::string source{ "some string to search in"};
         std::string to_find{ "some"};


         EXPECT_TRUE( std::search( std::begin( source), std::end( source), std::begin( to_find), std::end( to_find)) != std::end( source));
      }


      TEST( common_conformance, posix_spawnp)
      {
         common::unittest::Trace trace;

         // We don't want any sig-child
         signal::thread::scope::Block block{ { code::signal::child}};


         auto pids = algorithm::generate_n< 30>( []()
         {
            pid_t pid{};
            std::vector< const char*> arguments{ "sleep", "20", nullptr};

            auto current = environment::variable::native::current();

            auto environment = algorithm::transform( current, []( auto& value){ return value.data();});
            environment.push_back( nullptr);

            if( posix_spawnp(
                  &pid,
                  "sleep",
                  nullptr,
                  nullptr,
                  const_cast< char* const*>( arguments.data()),
                  const_cast< char* const*>( environment.data())) == 0)
            {
               return strong::process::id{ pid};
            }
            return strong::process::id{};
         });

         {
            auto signaled = decltype( pids){};

            for( auto pid : pids)
               if( kill( pid.value(), SIGINT) == 0)
                  signaled.push_back( pid);

            EXPECT_TRUE( signaled == pids);
         }

         {
            auto terminated = decltype( pids){};

            for( auto pid : pids)
            {
               auto result = waitpid( pid.value(), nullptr, 0);

               EXPECT_TRUE( result == pid.value()) << string::compose( "result: ", result, " - pid: ", pid, " - errno: ", common::code::system::last::error());

               if( result == pid.value())
                  terminated.push_back( pid);
            }
            EXPECT_TRUE( terminated == pids) << string::compose( "terminated: ", terminated, ", pids: ", pids);
         }
      }



      namespace local
      {
         namespace
         {
            int handle_exception()
            {
               try 
               {
                  throw;
               }
               catch( int value)
               {
                  return value;
               }
               catch( ...)
               {
                  return 0;
               }
            }

            void handle_exception_rethrow( int value)
            {
               try 
               {
                  throw value;
               }
               catch( ...)
               {
                  EXPECT_TRUE( handle_exception() == value);
                  throw;
               }
            }
            
         } // <unnamed>
      } // local

      TEST( common_conformance, nested_handled_exception_retrown)
      {
         try 
         {
            local::handle_exception_rethrow( 42);
         }
         catch( ...)
         {
            EXPECT_TRUE( local::handle_exception() == 42);
         }
      }


      TEST( common_conformance_filesystem, directory_symlink)
      {
         common::unittest::Trace trace;

         namespace fs = std::filesystem;
         unittest::directory::temporary::Scoped base;
         fs::path base_path{ base.path()};
         ASSERT_TRUE( fs::create_directories( base_path / "origin"));

         fs::create_directory_symlink( base_path / "origin", base_path / "link");

         // already exists -> gives false
         EXPECT_TRUE( ! fs::create_directories(  base_path / "origin"));

         EXPECT_TRUE( fs::exists( base_path / "link"));
         
         // fails hard, since 'link' is a link... Why?!! 
         // EXPECT_TRUE( fs::create_directories(  base_path / "link"));

         EXPECT_NO_THROW({
            // "helper" that takes care of corner cases
            common::directory::create( base_path / "link");
         });

         // check if we can create a sub-directory in the linked directory
         EXPECT_TRUE( fs::create_directories(  base_path / "link/subdir"));

         {
            std::ofstream out{ base_path / "link/file"};
            EXPECT_TRUE( out);
         }

      }




      /*
       * generates error with -Werror=return-type
       *
      namespace local
      {
         namespace
         {
            bool ommit_return()
            {

            }
         } // <unnamed>
      } // local


      TEST( common_conformance, ommitt_return)
      {
         EXPECT_TRUE( local::ommit_return());
      }
      */



      


   } // common
} // casual
