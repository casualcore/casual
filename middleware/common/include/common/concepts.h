//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <iterator>

namespace casual
{
   namespace common::concepts
   {
      template< typename T>
      concept range = requires( T& value)
      {
         std::ranges::begin( value);
         std::ranges::end( value);
      };

      namespace container
      {
         template<typename T>
         concept insert = concepts::range< T> && 
         requires( T a)
         {
            a.insert( std::end( a), std::begin( a), std::end( a));
         };
         
      } // container
      
   } // common::concepts
} // casual
