//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <random>
#include <algorithm>

namespace casual
{
   namespace common::algorithm::random
   {
      std::mt19937& generator() noexcept;

      //! inserts `value` at a random position in `container`
      template< typename C, typename V>
      C& insert( C& container, V&& value)
      {
         std::uniform_int_distribution< decltype( container.size())> distribution{ 0, container.size()};

         container.insert( std::begin( container) + distribution( random::generator()), std::forward< V>( value));

         return container;
      }

      template< typename R>
      auto shuffle( R&& range)
      {
         std::ranges::shuffle( range, random::generator());
         return std::forward< R>( range);
      }
      
   } // common::algorithm::random
} // casual