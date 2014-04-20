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
#include <ostream>

#include <assert.h>

namespace casual
{
   namespace common
   {

      template< typename Enum>
      auto as_integer( Enum value) -> typename std::underlying_type< Enum>::type
      {
         return static_cast< typename std::underlying_type< Enum>::type>(value);
      }


      template< typename Iter>
      struct Range
      {
         using iterator = Iter;
         using value_type = typename std::iterator_traits< Iter>::value_type;

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

         explicit operator bool () const
         {
            return ! empty();
         }


         auto operator * () -> decltype( *std::declval< iterator>()) { return *first;}
         auto operator * () const -> decltype( *std::declval< iterator>()) { return *first;}


         iterator operator -> () { return first;}
         //const value_type* operator -> () const { return &(*first);}

         Range operator ++ ()
         {
            Range other{ *this};
            ++first;
            return other;
         }

         Range& operator ++ ( int)
         {
            ++first;
            return *this;
         }

         iterator begin() const { return first;}
         iterator end() const { return last;}

         iterator first;
         iterator last;
      };

      template< typename Iter>
      Range< Iter> operator + ( const Range< Iter>& lhs, const Range< Iter>& rhs)
      {
         Range< Iter> result( lhs);
         if( rhs.first < result.first) result.first = rhs.first;
         if( rhs.last > result.last) result = rhs.last;
         return result;
      }

      template< typename Iter>
      Range< Iter> operator - ( const Range< Iter>& lhs, const Range< Iter>& rhs)
      {
         Range< Iter> result{ lhs};
         if( rhs.last > result.first && rhs.last < result.last) result.first = rhs.last;

         if( rhs.first < result.last && rhs.first > result.first) result.last = rhs.first;

         return result;
      }


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


      template< typename T>
      struct Negate
      {
         /*
         template< typename F>
         Negate( F&& functor) : m_functor{ std::forward< F>( functor)}
         {

         }
         */

         Negate( T&& functor) : m_functor{ std::move( functor)}
         {

         }

         template< typename... Args>
         bool operator () ( Args&& ...args) const
         {
            return ! m_functor( std::forward< Args>( args)...);
         }

      private:
         T m_functor;
      };


      template< typename T>
      Negate< T> negate( T&& functor)
      {
         return Negate< T>{ std::forward< T>( functor)};
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

         template< typename Iter>
         Range< Iter> make( Range< Iter> range)
         {
            return range;
         }



         template< typename R, typename C>
         auto sort( R&& range, C compare) -> decltype( make( std::forward< R>( range)))
         {
            auto inputRange = make( std::forward< R>( range));
            std::sort( std::begin( inputRange), std::end( inputRange), compare);
            return inputRange;
         }

         template< typename R>
         auto sort( R&& range) -> decltype( make( std::forward< R>( range)))
         {
            auto inputRange = make( std::forward< R>( range));
            std::sort( std::begin( inputRange), std::end( inputRange));
            return inputRange;
         }

         template< typename R, typename C>
         auto stable_sort( R&& range, C compare) -> decltype( make( std::forward< R>( range)))
         {
            auto inputRange = make( std::forward< R>( range));
            std::stable_sort( std::begin( inputRange), std::end( inputRange), compare);
            return inputRange;
         }

         template< typename R>
         auto stable_sort( R&& range) -> decltype( make( std::forward< R>( range)))
         {
            auto inputRange = make( std::forward< R>( range));
            std::stable_sort( std::begin( inputRange), std::end( inputRange));
            return inputRange;
         }

         template< typename R, typename P>
         auto partition( R&& range, P predicate) -> decltype( make( std::forward< R>( range)))
         {
            auto inputRange = make( std::forward< R>( range));
            inputRange.last = std::partition( std::begin( inputRange), std::end( inputRange), predicate);
            return inputRange;
         }


         template< typename R, typename P>
         auto stable_partition( R&& range, P predicate) -> decltype( make( std::forward< R>( range)))
         {
            auto inputRange = make( std::forward< R>( range));
            inputRange.last = std::stable_partition( std::begin( inputRange), std::end( inputRange), predicate);
            return inputRange;
         }


         template< typename R, typename Iter2>
         void copy( R&& range, Iter2 output)
         {
            std::copy( std::begin( range), std::end( range), output);
         }


         /*
         template< typename InputRange, typename OutputRange, typename T>
         OutputRange transform( InputRange&& range, OutputRange&& output, T transform)
         {
            auto inputRange = make( std::forward< InputRange>( range));
            //assert( inputRange.size() <= output.size());
            std::transform( std::begin( inputRange), std::end( range), std::begin( output), transform);
            return output;
         }
         */


         template< typename R, typename C, typename T>
         auto transform( R&& range, C& container, T transform) -> decltype( make( container))
         {
            auto inputRange = make( std::forward< R>( range));
            std::transform( std::begin( inputRange), std::end( inputRange), std::back_inserter( container), transform);
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

         //!
         //! @return true if all elements in the range compare equal
         //!
         template< typename R>
         bool uniform( R&& range)
         {
            if( range.size() < 2)
            {
               return true;
            }

            auto current = range.first;

            while( current != range.last)
            {
               if( *current != *++current)
               {
                  return false;
               }
            }
            return true;
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

         template< typename R, typename T>
         auto find( R&& range, T&& value) -> decltype( make( range))
         {
            auto resultRange = make( std::forward< R>( range));
            resultRange.first = std::find( std::begin( resultRange), std::end( resultRange), std::forward< T>( value));
            return resultRange;
         }


         template< typename R, typename P>
         auto find_if( R&& range, P predicate) -> decltype( make( std::forward< R>( range)))
         {
            auto resultRange = make( std::forward< R>( range));
            resultRange.first = std::find_if( std::begin( resultRange), std::end( resultRange), predicate);
            return resultRange;
         }


         template< typename R, typename F>
         auto for_each( R&& range, F functor) -> decltype( make( std::forward< R>( range)))
         {
            auto resultRange = make( std::forward< R>( range));
            std::for_each( std::begin( resultRange), std::end( resultRange), functor);
            return resultRange;
         }


         template< typename R1, typename R2, typename F>
         auto find_first_of( R1&& target, R2&& source, F functor) -> decltype( make( target))
         {
            auto resultRange = make( target);

            resultRange.first = std::find_first_of(
                  std::begin( resultRange), std::end( resultRange),
                  std::begin( source), std::end( source), functor);

            return resultRange;
         }


         template< typename R1, typename R2>
         auto find_first_of( R1&& target, R2&& source) -> decltype( make( target))
         {
            auto resultRange = make( target);

            resultRange.first = std::find_first_of(
                  std::begin( resultRange), std::end( resultRange),
                  std::begin( source), std::end( source));

            return resultRange;
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
               return group( range, std::less< typename Range< Iter>::value_type>{});
            }

         } // sorted
      } // range


      namespace chain
      {
         namespace link
         {

            template< typename L, typename T1, typename T2>
            struct basic_link
            {
               using link_type = L;

               basic_link( T1&& left, T2&& right) : left( std::forward< T1>( left)), right( std::forward< T2>( right)) {}

               template< typename T>
               auto operator() ( T&& value) const -> decltype( link_type::link( std::declval< T1>(),  std::declval< T2>(), std::forward< T>( value)))
               {
                  return link_type::link( left, right, std::forward< T>( value));
               }
            private:
               T1 left;
               T2 right;
            };

            template< typename Link, typename Arg>
            Arg make( Arg&& param) //-> decltype( std::forward< Arg>( param))
            {
               return param; //std::forward< Arg>( param);
            }

            template< typename L, typename Arg, typename... Args>
            auto make( Arg&& param, Args&&... params) -> basic_link< L, Arg, decltype( make< L>( std::forward< Args>( params)...))>
            {
               using nested_type = decltype( make< L>( std::forward< Args>( params)...));
               return basic_link< L, Arg, nested_type>( std::forward< Arg>( param), make< L>( std::forward< Args>( params)...));
            }

            struct Nested
            {
               template< typename T1, typename T2, typename T>
               static auto link( T1&& left, T2&& right, T&& value) -> decltype( left( right( value)))
               {
                  return left( right( value));
               }
            };

            struct And
            {
               template< typename T1, typename T2, typename T>
               static auto link( T1&& left, T2&& right, T&& value) -> decltype( left( value) && right( value))
               {
                  return left( value) && right( value);
               }
            };

            struct Or
            {
               template< typename T1, typename T2, typename T>
               static auto link( T1&& left, T2&& right, T&& value) -> decltype( left( value) || right( value))
               {
                  return left( value) || right( value);
               }
            };

         } // link

         template< typename Link>
         struct basic_chain
         {
            template< typename... Args>
            static auto link( Args&&... params) -> decltype( link::make< Link>( std::forward< Args>( params)...))
            {
               return link::make< Link>( std::forward< Args>( params)...);
            }
         };


         using Nested = basic_chain< link::Nested>;
         using And = basic_chain< link::And>;
         using Or = basic_chain< link::Or>;


      } // chain

      namespace extract
      {
         struct Second
         {
            template< typename T>
            auto operator () ( T&& value) const -> decltype( value.second)
            {
               return value.second;
            }
         };

      }


   } // common


} // casual

namespace std
{
   template< typename Enum>
   typename enable_if< is_enum< Enum>::value, ostream&>::type
   operator << ( ostream& out, Enum value)
   {
     return out << casual::common::as_integer( value);
   }

} // std




#endif // ALGORITHM_H_
