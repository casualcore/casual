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
         typedef typename std::iterator_traits< Iter>::value_type value_type;

         Range( Iter first, Iter last) : first( first), last( last) {}

         auto size() const -> decltype( std::distance( Iter(), Iter()))
         {
            return std::distance( first, last);
         }

         bool empty() const
         {
            return first == last;
         }

         Iter begin() { return first;}
         Iter end() { return last;}

         const Iter begin() const { return first;}
         const Iter end() const { return last;}

         Iter first;
         Iter last;
      };

      template< typename Iter>
      std::ostream& operator << ( std::ostream& out, Range< Iter> range)
      {
         out << "[";
         while( range.first != range.last)
         {
            out << *range.first;
            if( range.first + 1 != range.last)
               out << ",";

            ++range.first;
         }
         out << "]";
         return out;
      }

      template< typename Iter>
      Range< Iter> make_range( Iter first, Iter last)
      {
         return Range< Iter>( first, last);
      }

      template< typename C>
      auto make_range( C& container) -> decltype( make_range( std::begin( container), std::end( container)))
      {
         return make_range( std::begin( container), std::end( container));
      }

      template< typename C>
      auto make_range( const C& container) -> decltype( make_range( std::begin( container), std::end( container)))
      {
         return make_range( std::begin( container), std::end( container));
      }

      template< typename C>
      auto make_reverse_range( C& container) -> decltype( make_range( container.rbegin(), container.rend()))
      {
         return make_range( container.rbegin(), container.rend());
      }

      template< typename C>
      auto make_reverse_range( const C& container) -> decltype( make_range( container.rbegin(), container.rend()))
      {
         return make_range( container.rbegin(), container.rend());
      }


      template< typename Iter, typename C>
      Range< Iter> sort( Range< Iter> range, C compare)
      {
         std::sort( std::begin( range), std::end( range), compare);
         return range;
      }

      template< typename Iter>
      Range< Iter> sort( Range< Iter> range)
      {
         std::sort( std::begin( range), std::end( range));
         return range;
      }

      template< typename Iter, typename P>
      Range< Iter> partition( Range< Iter> range, P predicate)
      {
         range.last = std::partition( std::begin( range), std::end( range), predicate);
         return range;
      }


      template< typename Iter, typename P>
      Range< Iter> stable_partition( Range< Iter> range, P predicate)
      {
         range.last = std::stable_partition( std::begin( range), std::end( range), predicate);
         return range;
      }


      template< typename Iter1, typename Iter2>
      void copy( Range< Iter1> range, Iter2 output)
      {
         std::copy( std::begin( range), std::end( range), output);
      }


      template< typename InputIter, typename OutputIter, typename T>
      OutputIter transform( Range< InputIter> range, OutputIter output, T transform)
      {
         return std::transform( std::begin( range), std::end( range), output, transform);
      }

      template< typename InputIter1, typename InputIter2, typename outputIter, typename T>
      outputIter transform( Range< InputIter1> range1, Range< InputIter2> range2, outputIter output, T transform)
      {
         return std::transform( std::begin( range1), std::end( range1), output, transform);
      }


      template< typename Iter1, typename Iter2, typename P>
      bool equal( Range< Iter1> lhs, Range< Iter2> rhs, P predicate)
      {
         if( lhs.size() != rhs.size())
         {
            return false;
         }
         return std::equal( std::begin( lhs), std::end( lhs), std::begin( rhs), predicate);
      }


      template< typename Iter1, typename Iter2>
      bool equal( Range< Iter1> lhs, Range< Iter2> rhs)
      {
         if( lhs.size() != rhs.size())
         {
            return false;
         }
         return std::equal( std::begin( lhs), std::end( lhs), std::begin( rhs));
      }

      template< typename Iter, typename T>
      typename std::enable_if< std::is_convertible< T, typename Range< Iter>::value_type>::value, Range< Iter>>::type
      find( Range< Iter> range, T vlaue)
      {
         Range< Iter> result{ range};
         result.first = std::find( range.first, range.last, vlaue);
         return result;
      }


      template< typename Iter, typename P>
      typename std::enable_if< ! std::is_convertible< P, typename Range< Iter>::value_type>::value, Range< Iter>>::type
      find( Range< Iter> range, P predicate)
      {
         Range< Iter> result{ range};
         result.first = std::find_if( range.first, range.last, predicate);
         return result;
      }




      namespace sorted
      {
         template< typename Iter, typename T, typename C>
         Range< Iter> bound( Range< Iter> range, const T& value, C compare)
         {
            range.first = std::lower_bound( range.first, range.last, value, compare);
            range.last = std::upper_bound( range.first, range.last, value, compare);

            return range;
         }


         template< typename Iter, typename T>
         Range< Iter> bound( Range< Iter> range, const T& value)
         {
            return bound( range, value, std::less< T>{});
         }


         template< typename Iter, typename C>
         std::vector< Range< Iter>> group( Range< Iter> range, C compare)
         {
            std::vector< Range< Iter>> result;

            while( ! range.empty())
            {
               result.push_back( bound( range, *range.first, compare));
               range.first = result.back().last;
            }

            return result;
         }

         template< typename Iter, typename T>
         std::vector< Range< Iter>> group( Range< Iter> range)
         {
            return group( range, std::less< typename std::iterator_traits< Iter>::value_type>{});
         }

      } // sorted
   } // common

} // casual

#endif // ALGORITHM_H_
