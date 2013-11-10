//!
//! algorithm.h
//!
//! Created on: Nov 10, 2013
//!     Author: Lazan
//!

#ifndef ALGORITHM_H_
#define ALGORITHM_H_


#include <algorithm>
#include <iterator>

namespace casual
{
   namespace common
   {

      template< typename Iter>
      struct Range
      {
         Range( Iter first, Iter last) : first( first), last( last) {}

         auto size() const -> decltype( std::distance( Iter(), Iter()))
         {
            return std::distance( first, last);
         }

         bool empty() const
         {
            return first == last;
         }

         Iter first;
         Iter last;
      };

      template< typename Iter>
      Range< Iter> make_range( Iter first, Iter last)
      {
         return Range< Iter>( first, last);
      }


      namespace sorted
      {


         template< typename Iter, typename T, typename C>
         Range< Iter> bound( Iter first, Iter last, const T& value, C compare)
         {
            first = std::lower_bound( first, last, value, compare);
            last = std::upper_bound( first, last, value, compare);

            return make_range( first, last);
         }

         template< typename Iter, typename T>
         Range< Iter> bound( Iter first, Iter last, const T& value)
         {
            return bound( first, last, value, std::less< T>{});
         }


         template< typename Iter, typename C>
         std::vector< Range< Iter>> group( Iter first, Iter last, C compare)
         {
            std::vector< Range< Iter>> result;

            while( first != last)
            {
               result.push_back( bound( first, last, *first, compare));
               first = result.back().last;
            }

            return result;
         }

         template< typename Iter, typename T>
         std::vector< Range< Iter>> group( Iter first, Iter last)
         {
            return group( first, last, std::less< typename std::iterator_traits< Iter>::value_type>{});
         }

      } // sorted
   } // common

} // casual

#endif // ALGORITHM_H_
