//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/range.h"
#include "common/transcode.h"


namespace casual
{
   namespace common
   {
      namespace view
      {
         template< typename Iter, std::enable_if_t< traits::is::binary::iterator< Iter>::value, int> = 0>
         struct basic_binary : common::Range< Iter> 
         {
            using common::Range< Iter>::Range;

            friend std::ostream& operator << ( std::ostream& out, const basic_binary& value)
            {
               return transcode::hex::encode( out, std::begin( value), std::end( value));
            }
         };
         
         namespace binary
         {
            namespace detail
            {
               template< typename Iter> 
               auto raw( Iter value) { return &( *value);}

               template< typename P> 
               auto convert( P* pointer) { return reinterpret_cast< platform::binary::pointer>( pointer);}

               template< typename P> 
               auto convert( const P* pointer) { return reinterpret_cast< platform::binary::immutable::pointer>( pointer);}


               template< typename Iter> 
               auto make( Iter first, Iter last)
               {
                  auto convert = []( auto p){ return detail::convert( raw( p));};
                  using pointer_type = decltype( convert( first));

                  return basic_binary< pointer_type>{ convert( first), convert( last)};
               }

            } // detail
            template< typename Iter, std::enable_if_t< traits::is::binary::iterator< Iter>::value, int> = 0>
            auto make( Iter first, Iter last)
            {
               return detail::make( first, last);
            }

            template< typename Iter, typename Count, std::enable_if_t< 
               traits::is::binary::iterator< Iter>::value 
               && std::is_integral< Count>::value, int> = 0>
            auto make( Iter first, Count count)
            {
               return make( first, first + count);
            }

            template< typename C, std::enable_if_t< std::is_lvalue_reference< C>::value && common::traits::is::binary::like< C>::value, int> = 0>
            auto make( C&& container)
            {
               return make( std::begin( container), std::end( container));
            }
         } // binary

         using Binary = decltype( binary::make( std::declval< platform::binary::type&>()));

         namespace immutable
         {
            using Binary = decltype( binary::make( std::declval< const platform::binary::type&>()));

         } // mutable

      } // view
   } // common
} // casual

