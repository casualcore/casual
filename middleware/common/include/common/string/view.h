//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <string_view>

//! TODO maintainence: remove on c++20

namespace casual
{
   namespace common::string::view
   {
      template< typename Iter, typename Sent>
      auto make( Iter first, Sent sentinel) -> decltype( std::string_view( &(*first), std::distance( first, sentinel)))
      {
         return std::string_view( &(*first), std::distance( first, sentinel));
      }

      template< typename R>
      auto make( R&& range) -> decltype( make( std::begin( range), std::end( range)))
      {
         return make( std::begin( range), std::end( range));
      }

      template< typename C>
      auto make( C* pointer) -> decltype( std::string_view{ pointer})
      {
         return std::string_view{ pointer};
      }
   } // common::string::view
} // casual