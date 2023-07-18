//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/range.h"
#include "common/algorithm/container.h"

#include <algorithm>

namespace casual
{
   namespace common::algorithm::sorted
   {
      template< typename R, typename T>
      bool search( R&& range, T&& value)
      {
         return std::binary_search( std::begin( range), std::end( range), std::forward< T>( value));
      }

      template< typename R, typename T, typename Compare>
      bool search( R&& range, T&& value, Compare compare)
      {
         return std::binary_search( std::begin( range), std::end( range), std::forward< T>( value), compare);
      }

      template< typename R1, typename R2, typename Output, typename Compare>
      Output& intersection( R1&& source, R2&& other, Output& result, Compare compare)
      {
         std::set_intersection(
               std::begin( source), std::end( source),
               std::begin( other), std::end( other),
               std::back_inserter( result),
               compare);

         return result;
      }

      template< typename R1, typename R2, typename Output>
      auto intersection( R1&& source, R2&& other, Output& result)
      {
         return intersection( std::forward< R1>( source), std::forward< R2>( other), result, std::less<>{});
      }



      template< typename R1, typename R2, typename Output, typename Compare>
      Output& difference( R1&& source, R2&& other, Output& result, Compare compare)
      {
         std::set_difference(
               std::begin( source), std::end( source),
               std::begin( other), std::end( other),
               std::back_inserter( result),
               compare);

         return result;
      }

      template< typename R1, typename R2, typename Output>
      auto difference( R1&& source, R2&& other, Output& result)
      {
         return difference( std::forward< R1>( source), std::forward< R2>( other), result, std::less<>{});
      }
      
      //! @returns a tuple with [first, lower_bound) and [lower_bound, last)
      //! @{
      template< typename R, typename T>
      auto lower_bound( R&& range, const T& value)
      {
         auto pivot = std::lower_bound( std::begin( range), std::end( range), value);
         return std::make_tuple( range::make( std::begin( range), pivot), range::make( pivot, std::end( range)));
      }

      template< typename R, typename T, typename Compare>
      auto lower_bound( R&& range, const T& value, Compare compare)
      {
         auto pivot = std::lower_bound( std::begin( range), std::end( range), value, compare);
         return std::make_tuple( range::make( std::begin( range), pivot), range::make( pivot, std::end( range)));
      }
      //! @}

      //! @returns a tuple with [first, upper_bound) and [upper_bound, last)
      template< typename R, typename T>
      auto upper_bound( R&& range, T&& value)
      {
         auto pivot = std::upper_bound( std::begin( range), std::end( range), value);
         return std::make_tuple( range::make( std::begin( range), pivot), range::make( pivot, std::end( range)));
      }

      //! @returns a tuple with [first, upper_bound) and [upper_bound, last)
      template< typename R, typename T, typename C>
      auto upper_bound( R&& range, T&& value, C compare)
      {
         auto pivot = std::upper_bound( std::begin( range), std::end( range), value, compare);
         return std::make_tuple( range::make( std::begin( range), pivot), range::make( pivot, std::end( range)));
      }         

      template< typename R, typename T, typename C>
      auto bound( R&& range, T&& value, C compare)
      {
         auto first = std::lower_bound( std::begin( range), std::end( range), value, compare);
         auto last = std::upper_bound( first, std::end( range), value, compare);

         return range::make( first, last);
      }

      template< typename R, typename T>
      auto bound( R&& range, T&& value)
      {
         return bound( std::forward< R>( range), value, std::less<>{});
      }

      //! Divide `a` in two parts [a-first, intersection-end), [intersection-end, a-last).
      //! @returns a tuple with two ranges (from `a`), where: 
      //!     * index 0 is the (sorted) _intersection_ of `a` and `b`
      //!     * index 1 is the (sorted) _difference_ of `a` and `b` 
      //! @attention both `a` and `b` has to be sorted. `a` is of course not guaranteed to be sorted after.
      template< typename A, typename B>
      auto intersection( A& a, B&& b) noexcept
      {
         auto source = range::make( a);

         constexpr auto make_result = []( auto& range, auto& source) noexcept
         {
            std::sort( std::begin( source), std::end( range));
            return std::make_tuple( 
               range::make( std::begin( range), std::begin( source)), 
               range::make( std::begin( source), std::end( range)));
         };

         for( auto& value : b)
         {
            if( auto found = std::get< 1>( lower_bound( source, value)))
            {
               auto last = std::rotate( std::begin( source), std::begin( found), std::end( source));
               auto first = std::find_if( std::begin( source), last, [ &origin = value]( auto& value){ return value != origin;});
               source = range::make( first, last);
            }
            else
               return make_result( a, source);
         }
         return make_result( a, source);
      }
      
      //! Get the unique difference between `source` `target`, and appends 
      //! this to `target`
      //! @returns perfect forward `target`.
      //! @attention `source` is most likely mutated during this algorithm.
      template< typename S, typename T>
      auto append_unique( S&& source, T&& target) -> decltype( algorithm::sort( std::forward< T>( target)))
      {
         auto difference = algorithm::unique( std::get< 1>( sorted::intersection( source, target)));  
         algorithm::container::append( difference, target);
         return algorithm::sort( std::forward< T>( target));
      }


      template< typename R, typename C>
      auto group( R&& range, C compare)
      {
         std::vector< range::type_t< R>> result;

         auto current = range::make( range);

         while( current)
         {
            result.push_back( bound( current, *current, compare));
            current.advance( result.back().size());
         }

         return result;
      }

      template< typename R, typename T>
      auto group( R&& range)
      {
         return group( std::forward< R>( range), std::less< typename R::value_type>{});
      }

      //! Returns a subrange that consists of (the first series of) values that fulfills the predicate
      template< typename R, typename P>
      auto subrange( R&& range, P&& predicate)
      {
         auto first = std::find_if( std::begin( range), std::end( range), predicate);
         return range::make( first, std::find_if( first, std::end( range), predicate::negate( predicate)));
      }

   } // common::algorithm::sorted
} // casual