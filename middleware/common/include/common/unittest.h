//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/unittest/log.h"

#include "casual/platform.h"
#include "common/serialize/macro.h"
#include "common/message/type.h"
#include "common/execute.h"
#include "common/exception/handle.h"


#include <array>
#include <iostream>

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         using size_type = platform::size::type;

            
         struct Message : common::message::basic_message< common::message::Type::unittest_message>
         {
            Message();

            //! Will adjust the size of the paylad, and exclude the size of
            //! the 'payload size'.
            //!
            //! payload-size = size - sizeof( platform::binary::type::size_type)
            //!
            //! @param size the size of what is transported.
            Message( size_type size);

            //! @return the transport size
            //!   ( payload-size + sizeof( platform::binary::type::size_type)
            size_type size() const;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               // we don't serialize execution
               //base_type::serialize( archive);
               CASUAL_SERIALIZE( payload);
            })

            // cores on ubuntu 16.04 when size gets over 5M.
            // guessing that the stack gets to big...
            // We switch to a vector instead.
            // std::array< char, size> payload;
            platform::binary::type payload;
         };


         namespace random
         {
            namespace detail
            {
               long long integer();
            } // detail

            unittest::Message message( size_type size);

            platform::binary::type binary( size_type size);
            std::string string( size_type size);


            platform::binary::type::value_type byte();

            template< typename R>
            decltype( auto) range( R&& range)
            {
               for( auto& value : range)
               {
                  value = byte();
               }
               return std::forward< R>( range);
            }

            template< typename Integer>
            auto integer()
            {
               return static_cast< Integer>( detail::integer());
            }

            template< typename T> 
            auto set( T& value) -> decltype( void( value = integer< T>()), void())
            {
               value = integer< T>();
            }


         } // random


         namespace to
         {
            template< typename T>
            auto vector( std::initializer_list< T> values)
            {
               return std::vector< T>{ std::move( values)};
            }
         } // to


         namespace capture
         {
            namespace output
            {
               //! capture source to target
               //! @param source the stream to capture
               //! @param target the stream that is used instead
               //! @returns an execution scope that restores the source stream at end of scope
               inline auto stream( std::ostream& source, std::ostream& target)
               {
                  auto origin = source.rdbuf( target.rdbuf());

                  return execute::scope( [origin, &source](){
                     source.rdbuf( origin);
                  });
               }
            } // outpout

            namespace standard
            {
               inline auto out( std::ostream& out)
               {
                  return capture::output::stream( std::cout, out);
               }

            } // standard
         } // capture

         namespace detail
         {
            template< typename A, typename C> 
            auto expect_code( A&& action, C code) -> decltype( ::testing::AssertionSuccess())
            {
               try 
               {
                  action();
                  return ::testing::AssertionFailure() << "no std::error_code was throwned\n";
               }
               catch( ...)
               {
                  auto condition = exception::code();

                  if( condition == code)
                     return ::testing::AssertionSuccess();

                  return ::testing::AssertionFailure() << "expected: " << code << " - got: " << condition << '\n';
               }
            }
            
         } // detail

      } // unittest
   } // common
} // casual

#define EXPECT_CODE( action, code_value)                                                  \
casual::common::unittest::detail::expect_code( [&](){ action;}, code_value)


#define ASSERT_CODE( action, code_value)                                                  \
try                                                                                       \
{                                                                                         \
   action                                                                                 \
   FAIL() << "no std::error_code was throwned";                                           \
}                                                                                         \
catch( ...)                                                                               \
{                                                                                         \
   auto condition = ::casual::common::exception::code();                                  \
   ASSERT_TRUE( condition == code_value) << "expected: " << code_value << " - got: " << condition;    \
}





