//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/unittest.h"

#include "common/stream.h"
#include "common/algorithm.h"
#include "common/traits.h"
#include "common/signal.h"
#include "common/code/system.h"
#include "common/strong/type.h"

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
      // static test of traits

      static_assert( traits::is_same< traits::remove_cvref_t< char const&>, char>::value, "traits::remove_cvref_t does not work...");

      static_assert( traits::is::string::like< decltype( "some string")>::value, "traits::is::string::like does not work...");

      static_assert( traits::is::string::like< char const (&)[7]>::value, "traits::is::string::like does not work...");
      static_assert( traits::is::string::like< char[ 20]>::value, "traits::is::string::like does not work...");

      static_assert( ! traits::is::string::like< char* [ 20]>::value, "traits::is::string::like does not work...");


      static_assert( traits::is::iterable< char[ 20]>::value, "traits::is::iterable does not work...");

      static_assert( ! traits::is_any< char, unsigned char, signed char>::value, "traits::is_any does not work...");
      static_assert( traits::is_any< char, unsigned char, signed char, char>::value, "traits::is_any does not work...");

      static_assert( traits::is::tuple< std::pair< int, long>>::value, "traits::is::tuple does not work...");

      TEST( common_conformance, struct_with_pod_attributes__is_pod)
      {
         struct POD
         {
            int int_value;
         };

         EXPECT_TRUE( std::is_pod< POD>::value);
      }

      TEST( common_conformance, struct_with_member_function__is_pod)
      {
         struct POD
         {
            int func1() { return 42;}
         };

         EXPECT_TRUE( std::is_pod< POD>::value);
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

         //
         // We don't want any sig-child
         //
         signal::thread::scope::Block block{ { code::signal::child}};

         const char* const arguments[] = { "sleep", "20", nullptr };

         const std::size_t count = 30;
         std::vector< pid_t> pids;

         for( auto counter = count; counter > 0; --counter)
         {
            pid_t pid;

            if( posix_spawnp(
                  &pid,
                  "sleep",
                  nullptr,
                  nullptr,
                  const_cast< char* const*>( arguments),
                  nullptr) == 0)
            {
               pids.push_back( pid);
            }
         }

         EXPECT_TRUE( pids.size() == count);

         {
            std::vector< pid_t> signaled;

            for( auto pid : pids)
            {
               if( kill( pid, SIGINT) == 0)
               {
                  signaled.push_back( pid);
               }
            }

            EXPECT_TRUE( signaled == pids);
         }

         {
            std::vector< pid_t> terminated;

            for( auto pid : pids)
            {
               auto result = waitpid( pid, nullptr, 0);

               EXPECT_TRUE( result == pid) << "result: " << result << " - pid: " << pid << " - errno: " << common::code::system::last::error();

               if( result == pid)
               {
                  terminated.push_back( pid);
               }
            }
            EXPECT_TRUE( terminated == pids) << "terminated: " << terminated << ", pids: " << pids;
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
