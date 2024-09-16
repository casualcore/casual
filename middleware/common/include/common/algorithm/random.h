//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/concepts.h"
#include "common/range.h"

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

      template< concepts::range R>
      auto shuffle( R&& range)
      {
         std::ranges::shuffle( range, random::generator());
         return std::forward< R>( range);
      }

      template< typename T, T min = std::numeric_limits< T>::min(), T max = std::numeric_limits< T>::max()> 
      requires concepts::any_of< T, short, int, long, long long, unsigned short, unsigned int, unsigned long, unsigned long long>
      T value()
      {
         return std::uniform_int_distribution< T>{ min, max}( random::generator());
      }

      //! @returns a range with size 1 where begin is a random place in `range`
      auto next( concepts::range auto&& range) -> decltype( range::make( range))
      {
         if( std::empty( range))
            return {};

         auto distribution = std::uniform_int_distribution< platform::size::type>{ 0, std::ssize( range) - 1};
         auto begin = std::next( std::begin( range), distribution( random::generator()));

         return range::make( begin, std::next( begin));
      }
      
   } // common::algorithm::random
} // casual