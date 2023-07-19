//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/range.h"
#include "casual/concepts.h"
#include "common/transcode.h"


namespace casual
{
   namespace common
   {
      namespace view
      {
         template< typename T>
         struct basic_binary : common::Range< T> 
         {
            using common::Range< T>::Range;

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
                  using value_type = std::remove_reference_t< decltype( convert( first))>;

                  return basic_binary< value_type>{ convert( first), convert( last)};
               }

            } // detail
            template< concepts::binary::iterator Iter>
            auto make( Iter first, Iter last)
            {
               return detail::make( first, last);
            }

            template< concepts::binary::iterator Iter, std::integral Count>
            auto make( Iter first, Count count)
            {
               return make( first, first + count);
            }

            template< concepts::binary::like C>
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

