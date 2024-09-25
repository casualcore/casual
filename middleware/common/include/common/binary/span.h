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
   namespace common::binary::span
   {
      namespace strong
      {
         template< concepts::binary::value_type T, typename Tag>
         struct Span : std::span< T>
         {
            using base_type = std::span< T>;
            using base_type::base_type;

            explicit Span( std::span< T> value) : base_type( value) {}
         };

      } // strong


      //! sort of hack to get the std::byte span to a string like
      //! span, compatible with platform::binary::type std::vector< char>...
      //!  The goal is to replace std::vector< char> with std::vector< std::byte>
      //!  But for now, we need to play nice with platform::binary::type
      //! @{
      inline auto to_string_like( std::span< const std::byte> span)
      {
         return std::span< const char>{ reinterpret_cast< const char*>( std::data( span)), span.size()};
      }

      inline auto to_string_like( std::span< std::byte> span)
      {
         return std::span< char>{ reinterpret_cast< char*>( std::data( span)), span.size()};
      }

      inline auto to_string_like( platform::binary::type& container)
      {
         return to_string_like( std::span< std::byte>{ container});
      }
      //! @}
      
      namespace detail
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

         struct fixed_span_tag{};
         
      } // detail

      //! represent a static/fixed size binary span only for
      //! std::byte and const std::byte
      template< typename T>
      using Fixed = strong::Span< T, detail::fixed_span_tag>;

      namespace fixed
      {
         template< detail::compatible::iterator Iter>
         auto make( Iter first, Iter last)
         {
            auto span = std::span{ first, last};

            using value_type = std::remove_reference_t< decltype( span[ 0])>;

            if constexpr( std::is_const_v< value_type>)
            {
               auto bytes = std::as_bytes( span);
               return Fixed< const std::byte>( std::begin( bytes), std::end( bytes));
            }
            else
            {
               auto bytes = std::as_writable_bytes( span);
               return Fixed< std::byte>( std::begin( bytes), std::end( bytes));
            }
         }

         template< detail::compatible::iterator Iter, std::integral Count>
         auto make( Iter first, Count count)
         {
            return make( first, first + count);
         }

         template< detail::compatible::range C>
         auto make( C& container)
         {
            static_assert( concepts::container::array< C>);
            return fixed::make( std::begin( container), std::end( container));
         }  
      } // fixed


      template< detail::compatible::iterator Iter>
      auto make( Iter first, Iter last)
      {
         auto span = std::span{ first, last};

         using value_type = std::remove_reference_t< decltype( span[ 0])>;

         if constexpr( std::is_const_v< value_type>)
            return std::as_bytes( span);
         else
            return std::as_writable_bytes( span);
      }

      template< detail::compatible::iterator Iter, std::integral Count>
      auto make( Iter first, Count count)
      {
         return make( first, first + count);
      }

      template< detail::compatible::range C>
      auto make( C&& container)
      {
         return make( std::begin( container), std::end( container));
      }


   } // common::binary::span
} // casual

