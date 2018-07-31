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
         template< typename Iter>
         struct Binary : common::Range< Iter> 
         {
            using common::Range< Iter>::Range;

            friend std::ostream& operator << ( std::ostream& out, const Binary& value)
            {
               return transcode::hex::encode( out, std::begin( value), std::end( value));
            }
         };
         
         namespace binary
         {
            template< typename Iter, typename = std::enable_if_t< common::traits::is::iterator< Iter>::value>>
            Binary< Iter> make( Iter first, Iter last)
            {
               return Binary< Iter>( first, last);
            }

            template< typename Iter, typename Count, std::enable_if_t< 
               common::traits::is::iterator< Iter>::value 
               && std::is_integral< Count>::value>* dummy = nullptr>
            auto make( Iter first, Count count)
            {
               return make( first, first + count);
            }

            template< typename C, typename = std::enable_if_t<std::is_lvalue_reference< C>::value && common::traits::is::iterable< C>::value>>
            auto make( C&& container)
            {
               return make( std::begin( container), std::end( container));
            }
            
         } // binary

      } // view
   } // common
} // casual

