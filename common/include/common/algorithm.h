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
#include <type_traits>

#include <assert.h>

namespace casual
{
   namespace common
   {

      template< typename Iter>
      struct Range
      {
         typedef typename std::iterator_traits< Iter>::value_type value_type;

         Range() : last( first) {}
         Range( Iter first, Iter last) : first( first), last( last) {}

         auto size() const -> decltype( std::distance( Iter(), Iter()))
         {
            return std::distance( first, last);
         }

         bool empty() const
         {
            return first == last;
         }


         Iter begin() const { return first;}
         Iter end() const { return last;}

         Iter first;
         Iter last;
      };

      template< typename Iter>
      std::ostream& operator << ( std::ostream& out, Range< Iter> range)
      {
         if( out.good())
         {
            out << "[";
            while( range.first != range.last)
            {
               out << *range.first++;
               if( range.first != range.last)
                  out << ",";
            }
            out << "]";
         }
         return out;
      }



      namespace range
      {
         template< typename Iter>
         Range< Iter> make( Iter first, Iter last)
         {
            return Range< Iter>( first, last);
         }

         template< typename C, class = typename std::enable_if<std::is_lvalue_reference< C>::value>::type >
         auto make( C&& container) -> decltype( make( std::begin( container), std::end( container)))
         {
            return make( std::begin( container), std::end( container));
         }


         template< typename C, class = typename std::enable_if<std::is_lvalue_reference< C>::value>::type >
         auto make_reverse( C&& container) -> decltype( make( container.rbegin(), container.rend()))
         {
            return make( container.rbegin(), container.rend());
         }


         template< typename R, typename C>
         R sort( R range, C compare)
         {
            std::sort( std::begin( range), std::end( range), compare);
            return range;
         }

         template< typename R>
         R sort( R range)
         {
            std::sort( std::begin( range), std::end( range));
            return range;
         }

         template< typename Iter, typename C>
         Range< Iter> stable_sort( Range< Iter> range, C compare)
         {
            std::stable_sort( std::begin( range), std::end( range), compare);
            return range;
         }

         template< typename Iter>
         Range< Iter> stable_sort( Range< Iter> range)
         {
            std::stable_sort( std::begin( range), std::end( range));
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
         Range< OutputIter> transform( Range< InputIter> range, Range< OutputIter> output, T transform)
         {
            assert( range.size() <= output.size());
            std::transform( std::begin( range), std::end( range), std::begin( output), transform);
            return output;
         }


         template< typename Iter, typename C, typename T>
         auto transform( Range< Iter> range, C& container, T transform) -> decltype( make( container))
         {
            std::transform( std::begin( range), std::end( range), std::back_inserter( container), transform);
            return make( container);
         }

         template< typename InputIter1, typename InputIter2, typename outputIter, typename T>
         outputIter transform( Range< InputIter1> range1, Range< InputIter2> range2, outputIter output, T transform)
         {
            assert( range1.size() == range2.size());
            return std::transform(
               std::begin( range1), std::end( range1),
               std::begin( range2),
               output, transform);
         }


         template< typename Iter>
         Range< Iter> unique( Range< Iter> range)
         {
            range.last = std::unique( range.first, range.last);
            return range;
         }

         template< typename C, typename Iter>
         Range< Iter> trim( C& container, Range< Iter> range)
         {
            auto index = range.first -  std::begin( container);
            container.erase( range.last, std::end( container));
            container.erase( std::begin( container), std::begin( container) + index);
            return make( container);
         }


         template< typename C, typename Iter>
         Range< Iter> erase( C& container, Range< Iter> range)
         {
            container.erase( range.first, range.last);
            return make( container);
         }


         template< typename R1, typename R2, typename P>
         bool equal( R1&& lhs, R2&& rhs, P predicate)
         {
            if( lhs.size() != rhs.size())
            {
               return false;
            }
            return std::equal( std::begin( lhs), std::end( lhs), std::begin( rhs), predicate);
         }


         template< typename R1, typename R2>
         bool equal( R1&& lhs, R2&& rhs)
         {
            if( lhs.size() != rhs.size())
            {
               return false;
            }
            return std::equal( std::begin( lhs), std::end( lhs), std::begin( rhs));
         }


         template< typename R, typename P>
         bool all_of( R&& range, P predicate)
         {
            return std::all_of( std::begin( range), std::end( range), predicate);
         }

         template< typename R, typename P>
         bool any_of( R&& range, P predicate)
         {
            return std::any_of( std::begin( range), std::end( range), predicate);
         }

         template< typename Iter, typename T>
         //typename std::enable_if< std::is_convertible< T, typename Range< Iter>::value_type>::value, Range< Iter>>::type
         Range< Iter> find( Range< Iter> range, T&& value)
         {
            range.first = std::find( range.first, range.last, std::forward< T>( value));
            return range;
         }


         template< typename Iter, typename P>
         //typename std::enable_if< ! std::is_convertible< P, typename Range< Iter>::value_type>::value, Range< Iter>>::type
         Range< Iter> find_if( Range< Iter> range, P predicate)
         {
            range.first = std::find_if( range.first, range.last, predicate);
            return range;
         }


         template< typename R, typename F>
         R for_each( R range, F functor)
         {
            std::for_each( std::begin( range), std::end( range), functor);
            return range;
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
      } // range
   } // common

} // casual

#endif // ALGORITHM_H_
