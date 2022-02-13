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
#include "common/code/raise.h"
#include "common/compare.h"
#include "common/communication/ipc.h"

#include <array>
#include <iostream>

namespace casual
{
   namespace common::unittest
   {
      using base_message = common::message::basic_message< common::message::Type::unittest_message>;
      struct Message : base_message, Compare< Message>
      {
         Message();
         Message( platform::binary::type payload);

         //! Will randomize the payload to the given `size`
         Message( platform::size::type size);

         CASUAL_CONST_CORRECT_SERIALIZE(
            base_message::serialize( archive);
            CASUAL_SERIALIZE( payload);
         )

         platform::binary::type payload;

         inline auto tie() const noexcept { return std::tie( correlation, payload);}
      };

      namespace message::transport
      {
         //! @returns a messages with a random payload. `size` dictates the transport size,
         //! the message.payload will be smaller than `size`.
         //! ( payload.size + sizeof( platform::size::type) + execution-id (16B) == size)
         unittest::Message size( platform::size::type size);
         
      } // message::transport


      namespace random
      {
         namespace detail
         {
            long long integer();
         } // detail

         platform::binary::type binary( platform::size::type size);
         std::string string( platform::size::type size);


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

      namespace fetch
      {
         //! tries to fetch and compare the predicate until the predicate returns true
         //! or we have reached 2k tries and a total time of ~16s, which should be enough for
         //! "all" the systems we're building casual on.
         template< typename F>
         constexpr auto until( F fetcher)
         {
            return [fetcher]( auto&& predicate)
            {
               constexpr auto total_count = 2000;
               auto state = fetcher();
               auto count = total_count;

               while( ! predicate( state) && --count > 0)
               {
                  common::process::sleep( std::chrono::milliseconds{ 8});
                  state = fetcher();
               }

               if( count == 0)
                  code::raise::error( code::casual::invalid_semantics, "unittest::fetch::until failed to fullfill the predicate after ", total_count, " tries");

               return state;
            };
         }
      } // fetch

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
               auto error = exception::capture();

               if( error.code() == code)
                  return ::testing::AssertionSuccess();

               return ::testing::AssertionFailure() << "expected: " << code << " - got: " << error.code() << '\n';
            }
         }
         
      } // detail

   } // common::unittest
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
   auto error = ::casual::common::exception::capture();                                  \
   ASSERT_TRUE( error.code() == code_value) << "expected: " << code_value << " - got: " << error.code();    \
}





