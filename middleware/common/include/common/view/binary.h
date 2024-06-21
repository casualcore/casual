//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/range.h"
#include "casual/concepts.h"

#include <span>

namespace casual
{
   namespace common::view
   {

      using Binary = std::span< std::byte>;

      namespace immutable
      {
         using Binary = std::span< const std::byte>;

      } // mutable

      namespace binary
      {
         namespace compatible
         {
            template< typename T>
            concept value_type = requires( T value) 
            { 
               sizeof( value) == 1;
            };

            template< typename T>
            concept iterator = std::contiguous_iterator< T> && value_type< std::iter_value_t< T>>;

            template< typename T>
            concept range = concepts::range< T> && requires( const T& a)
            {
               { *std::begin( a)} -> value_type; 
            };

         } // compatible


         //! sort of hack to get the std::byte span to a string like
         //! span, compatible with platform::binary::type std::vector< char>...
         //!  The goal is to replace std::vector< char> with std::vector< std::byte>
         //!  But for now, we need to play nice with platform::binary::type
         //! @{
         inline auto to_string_like( immutable::Binary span)
         {
            return std::span< const char>{ reinterpret_cast< const char*>( std::data( span)), span.size()};
         }

         inline auto to_string_like( Binary span)
         {
            return std::span< char>{ reinterpret_cast< char*>( std::data( span)), span.size()};
         }

         inline auto to_string_like( platform::binary::type& container)
         {
            return to_string_like( Binary{ container});
         }
         //! @}

         template< compatible::iterator Iter>
         auto make( Iter first, Iter last)
         {
            using value_type = std::remove_reference_t< decltype( *std::declval< Iter>())>;

            auto span = std::span{ first, last};

            if constexpr( std::is_const_v< value_type>)
               return std::as_bytes( span);
            else
               return std::as_writable_bytes( span);
         }

         template< compatible::iterator Iter, std::integral Count>
         auto make( Iter first, Count count)
         {
            return make( first, first + count);
         }

         template< compatible::range C>
         auto make( C&& container)
         {
            return make( std::begin( container), std::end( container));
         }
      } // binary


   } // common::view
} // casual

