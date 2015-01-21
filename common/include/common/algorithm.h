//!
//! algorithm.h
//!
//! Created on: Nov 10, 2013
//!     Author: Lazan
//!

#ifndef ALGORITHM_H_
#define ALGORITHM_H_


#include "common/traits.h"
#include "common/error.h"

#include <algorithm>
#include <numeric>
#include <iterator>
#include <type_traits>
#include <ostream>
#include <sstream>

#include <assert.h>
#include <functional>

namespace casual
{
   namespace common
   {
      namespace scope
      {
         struct Execute
         {
            Execute( std::function< void()> executer) : m_execute( std::move( executer)) {}

            ~Execute()
            {
               if( m_execute)
               {
                  try
                  {
                     m_execute();
                  }
                  catch( ...)
                  {
                     error::handler();
                  }
               }
            }

            void release()
            {
               m_execute = nullptr;
            }

            std::function< void()> m_execute;
         };
      } // scope

      namespace chain
      {
         namespace link
         {

            template< typename L, typename T1, typename T2>
            struct basic_link
            {
               using link_type = L;

               basic_link( T1&& left, T2&& right) : left( std::forward< T1>( left)), right( std::forward< T2>( right)) {}

               template< typename ...Values>
               auto operator() ( Values&& ...values) const -> decltype( link_type::link( std::declval< T1>(),  std::declval< T2>(), std::forward< Values>( values)...))
               {
                  return link_type::link( left, right, std::forward< Values>( values)...);
               }

               template< typename ...Values>
               auto operator() ( Values&& ...values) -> decltype( link_type::link( std::declval< T1>(),  std::declval< T2>(), std::forward< Values>( values)...))
               {
                  return link_type::link( left, right, std::forward< Values>( values)...);
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

            struct Order
            {
               template< typename T1, typename T2, typename V1, typename V2>
               static auto link( T1&& left, T2&& right, V1&& lhs, V2&& rhs) -> decltype( left( lhs, rhs))
               {
                  if( left( lhs, rhs))
                     return true;
                  if( left( rhs, lhs))
                     return false;
                  return right( lhs, rhs);
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
         using Order = basic_chain< link::Order>;


      } // chain

      namespace extract
      {
         struct Get
         {
            template< typename T>
            auto operator () ( T&& value) const -> decltype( value.get())
            {
               return value.get();
            }

         };


         struct Second
         {
            template< typename T>
            auto operator () ( T&& value) const -> decltype( value.second)
            {
               return value.second;
            }
         };

      }

      namespace compare
      {
         template< typename T>
         auto inverse( T&& functor) -> decltype( std::bind( std::forward< T>( functor), std::placeholders::_2, std::placeholders::_1))
         {
            return std::bind( std::forward< T>( functor), std::placeholders::_2, std::placeholders::_1);
         }

         template< typename T>
         struct Inverse
         {
            template< typename ...Args>
            Inverse( Args&&... args) : m_functor( std::forward< Args>( args)...) {}

            template< typename V1, typename V2>
            auto operator () ( V1&& lhs, V2&& rhs) const -> decltype( std::declval< T>()( std::forward< V2>( rhs), std::forward< V1>( lhs)))
            {
               return m_functor( std::forward< V2>( rhs), std::forward< V1>( lhs));
            }

            template< typename V1, typename V2>
            auto operator () ( V1&& lhs, V2&& rhs) -> decltype( std::declval< T>()( std::forward< V2>( rhs), std::forward< V1>( lhs)))
            {
               return m_functor( std::forward< V2>( rhs), std::forward< V1>( lhs));
            }

         private:
            T m_functor;
         };

      } // compare


      struct equal_to
      {
         template< typename T1, typename T2>
         constexpr auto operator()( const T1& lhs, const T2& rhs ) const -> decltype( lhs == rhs)
         {
            return lhs == rhs;
         }
      };


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

         std::size_t size() const
         {
            return std::abs( last - first);
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


         value_type* data()
         {
            if( ! empty())
            {
               return &( *first);
            }
            return nullptr;
         }

         const value_type* data() const
         {
            if( ! empty())
            {
               return &( *first);
            }
            return nullptr;
         }

         iterator first;
         iterator last;


         //template< typename Iter>
         friend Range< Iter> operator + ( const Range& lhs, const Range& rhs)
         {
            Range result( lhs);
            if( rhs.first < result.first) result.first = rhs.first;
            if( rhs.last > result.last) result.last = rhs.last;
            return result;
         }

         /*
         friend Range< Iter>& operator -= ( Range& lhs, const Range& rhs)
         {


         }
         */


         //template< typename Iter>
         friend Range< Iter> operator - ( const Range& lhs, const Range& rhs)
         {
            Range result{ lhs};

            //
            // TODO: make this better...

            if( rhs)
            {
               if( result.last > rhs.last)
               {
                  result.first = rhs.last;
               }
               if( result.first <= rhs.first)
               {
                  result.last = rhs.first;
               }
            }

            //
            // Make sure the ranges overlap
            //
            /*
            if( result.last > rhs.first && result.first < rhs.last)
            {
               if( result.last > rhs.first) result.last = rhs.first;
               else if( rhs.last > result.first) result.first = rhs.last;
            }
            */
            return result;
         }


         //template< typename Iter>
         friend std::ostream& operator << ( std::ostream& out, Range range)
         {
            if( out)
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
      };







      namespace detail
      {
         template< typename T>
         struct negate
         {

            negate( T&& functor) : m_functor{ std::move( functor)}
            {

            }

            template< typename... Args>
            bool operator () ( Args&& ...args) const
            {
               return ! m_functor( std::forward< Args>( args)...);
            }

            template< typename... Args>
            bool operator () ( Args&& ...args)
            {
               return ! m_functor( std::forward< Args>( args)...);
            }

         private:
            T m_functor;
         };

      } // detail



      template< typename T>
      detail::negate< T> negate( T&& functor)
      {
         return detail::negate< T>{ std::forward< T>( functor)};
      }


      //!
      //! This is not intended to be a serious attempt at a range-library
      //! Rather an abstraction that helps our use-cases and to get a feel for
      //! what a real range-library could offer. It's a work in progress
      //!
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

         template< typename C>
         struct traits
         {
            using type = decltype( make( C().begin(), C().end()));
         };



         template< typename R>
         auto to_vector( R&& range) -> std::vector< typename std::decay< decltype( *std::begin( range))>::type>
         {
            std::vector< typename std::decay< decltype( *std::begin( range))>::type> result;

            std::copy( std::begin( range), std::end( range), std::back_inserter( result));

            return result;
         }

         template< typename R>
         std::string to_string( R&& range)
         {
            std::ostringstream out;
            out << make( range);
            return out.str();
         }


         template< typename R>
         auto front( R&& range) -> decltype( make( std::forward< R>( range)))
         {
            auto result = make( std::forward< R>( range));
            if( result)
            {
               result.last = result.front + 1;
            }
            return result;
         }

         template< typename R>
         auto back( R&& range) -> decltype( make( std::forward< R>( range)))
         {
            auto result = make( std::forward< R>( range));
            if( result)
            {
               result.first = result.last - 1;
            }
            return result;
         }





         template< typename R, typename C>
         auto sort( R&& range, C compare) -> decltype( std::forward< R>( range))
         {
            std::sort( std::begin( range), std::end( range), compare);
            return std::forward< R>( range);
         }

         template< typename R>
         auto sort( R&& range) -> decltype( std::forward< R>( range))
         {
            std::sort( std::begin( range), std::end( range));
            return std::forward< R>( range);
         }

         template< typename R, typename C>
         auto stable_sort( R&& range, C compare) -> decltype( std::forward< R>( range))
         {
            std::stable_sort( std::begin( range), std::end( range), compare);
            return std::forward< R>( range);
         }

         template< typename R>
         auto stable_sort( R&& range) -> decltype( std::forward< R>( range))
         {
            std::stable_sort( std::begin( range), std::end( range));
            return std::forward< R>( range);
         }

         template< typename R, typename P>
         auto partition( R&& range, P predicate) -> decltype( std::make_tuple( make( std::forward< R>( range)), make( std::forward< R>( range))))
         {
            auto middle = std::partition( std::begin( range), std::end( range), predicate);
            return std::make_tuple( make( std::begin( range), middle), make( middle, std::end( range)));
         }


         template< typename R, typename P>
         auto stable_partition( R&& range, P predicate) -> decltype( std::make_tuple( make( std::forward< R>( range)), make( std::forward< R>( range))))
         {
            auto middle = std::stable_partition( std::begin( range), std::end( range), predicate);
            return std::make_tuple( make( std::begin( range), middle), make( middle, std::end( range)));
         }


         template< typename R, typename OutIter>
         OutIter copy( R&& range, OutIter output)
         {
            return std::copy( std::begin( range), std::end( range), output);
         }

         template< typename R, typename OutIter, typename P>
         OutIter copy_if( R&& range, OutIter output, P predicate)
         {
            return std::copy_if( std::begin( range), std::end( range), output, predicate);
         }

         template< typename R, typename O>
         O& append( R&& range, O& output)
         {
            output.insert( std::end( output), std::begin( range), std::end( range));
            return output;
         }


         template< typename R, typename Size, typename Iter>
         void copy_max( R&& range, Size size, Iter output)
         {
            auto inputRange = make( range);

            if( inputRange.size() <= size)
            {
               copy( inputRange, output);
            }
            else
            {
               std::copy_n( std::begin( inputRange), size, output);
            }
         }

         template< typename R, typename C>
         auto move( R&& range, C& container) -> decltype( make( container))
         {
            std::move( std::begin( range), std::end( range), std::back_inserter( container));
            return make( container);
         }


         template< typename R, typename C, typename T>
         auto transform( R&& range, C& container, T transform) -> decltype( make( container))
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


         //!
         //! Applies std::unique on [std::begin( range), std::end( range) )
         //!
         //! @return the unique range
         //!
         template< typename R>
         auto unique( R&& range) -> decltype( make( range))
         {
            auto resultRange = make( range);
            resultRange.last = std::unique( resultRange.first, resultRange.last);
            return resultRange;
         }

         //!
         //! Trims @p container so it matches @p range
         //!
         //! @return range that matches the trimmed @p container
         //!
         template< typename C, typename R>
         auto trim( C& container, R&& range) -> decltype( make( container))
         {
            auto index = range.first - std::begin( container);
            container.erase( range.last, std::end( container));
            container.erase( std::begin( container), std::begin( container) + index);
            return make( container);
         }


         template< typename C, typename Iter>
         auto erase( C& container, Range< Iter> range) -> decltype( make( container))
         {
            container.erase( range.first, range.last);
            return make( container);
         }

         template< typename R, typename P>
         auto remove_if( R&& range, P predicate) -> decltype( make( std::forward< R>( range)))
         {
            auto result = make( std::forward< R>( range));

            result.last = std::remove_if( std::begin( result), std::end( result), predicate);

            return result;
         }


         template< typename R1, typename R2, typename P>
         bool equal( R1&& lhs, R2&& rhs, P predicate)
         {
            //if( lhs.size() != rhs.size()) { return false;}
            return std::equal( std::begin( lhs), std::end( lhs), std::begin( rhs), predicate);
         }


         template< typename R1, typename R2>
         bool equal( R1&& lhs, R2&& rhs)
         {
            //if( lhs.size() != rhs.size()) { return false;}
            return std::equal( std::begin( lhs), std::end( lhs), std::begin( rhs));
         }


         template< typename R, typename T>
         auto accumulate( R&& range, T&& value) -> decltype( value + value)
         {
            return std::accumulate( std::begin( range), std::end( range), std::forward< T>( value));
         }

         template< typename R, typename T, typename F>
         auto accumulate( R&& range, T&& value, F&& functor) -> decltype( functor( std::forward< T>( value), *std::begin( range)))
         {
            return std::accumulate(
                  std::begin( range),
                  std::end( range),
                  std::forward< T>( value),
                  std::forward< F>( functor));
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


         template< typename R, typename F>
         auto for_each( R&& range, F functor) -> decltype( make( std::forward< R>( range)))
         {
            auto resultRange = make( std::forward< R>( range));
            std::for_each( std::begin( resultRange), std::end( resultRange), functor);
            return resultRange;
         }


         //!
         //! associate container specialization
         //!
         template< typename R, typename T, typename std::enable_if< common::traits::is_associative_container< typename std::decay<R>::type>::value>::type* = nullptr>
         auto find( R&& range, T&& value) -> decltype( make( std::forward< R>( range)))
         {
            auto resultRange = make( range);
            resultRange.first = range.find( value);
            return resultRange;
         }

         //!
         //! non associate container specialization
         //!
         template< typename R, typename T, typename std::enable_if< ! common::traits::is_associative_container< typename std::decay< R>::type>::value>::type* = nullptr>
         auto find( R&& range, T&& value) -> decltype( make( std::forward< R>( range)))
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


         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence that is equal to @p value
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename T>
         auto divide( R1&& range, T&& value) ->  decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            auto divider = std::find(
                  std::begin( range), std::end( range),
                  std::forward< T>( value));

            return std::make_tuple( make( std::begin( range), divider), make( divider, std::end( range)));
         }

         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence where @p predicate is true
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename P>
         auto divide_if( R1&& range, P predicate) ->  decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            auto divider = std::find_if( std::begin( range), std::end( range), predicate);

            return std::make_tuple( make( std::begin( range), divider), make( divider, std::end( range)));
         }



         template< typename R1, typename R2, typename F>
         auto find_first_of( R1&& target, R2&& source, F functor) -> decltype( make( std::forward< R1>( target)))
         {
            auto resultRange = make( std::forward< R1>( target));

            resultRange.first = std::find_first_of(
                  std::begin( resultRange), std::end( resultRange),
                  std::begin( source), std::end( source), functor);

            return resultRange;
         }


         template< typename R1, typename R2>
         auto find_first_of( R1&& target, R2&& source) -> decltype( make( std::forward< R1>( target)))
         {
            auto resultRange = make( std::forward< R1>( target));

            resultRange.first = std::find_first_of(
                  std::begin( resultRange), std::end( resultRange),
                  std::begin( source), std::end( source));

            return resultRange;
         }

         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence found in @p lookup
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename R2>
         auto divide_first( R1&& range, R2&& lookup) ->  decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            auto divider = std::find_first_of(
                  std::begin( range), std::end( range),
                  std::begin( lookup), std::end( lookup));

            return std::make_tuple( make( std::begin( range), divider), make( divider, std::end( range)));
         }

         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence found in @p lookup
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename R2, typename F>
         auto divide_first( R1&& range, R2&& lookup, F functor) ->  decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            auto divider = std::find_first_of(
                  std::begin( range), std::end( range),
                  std::begin( lookup), std::end( lookup), functor);

            return std::make_tuple( make( std::begin( range), divider), make( divider, std::end( range)));
         }


         //!
         //! Divide @p range in two parts [range-first, intersection-end), [intersection-end, range-last).
         //! where the first range is the intersection of the @p range and @p lookup
         //! and the second range is the complement of @range with regards to @p lookup
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename R2>
         auto intersection( R1&& range, R2&& lookup) -> decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            using range_type = decltype( make( std::forward< R1>( range)));
            using value_type = typename range_type::value_type;

            auto lambda = [&]( const value_type& value){ return find( lookup, value);};
            return stable_partition( std::forward< R1>( range), lambda);
         }

         //!
         //! Divide @p range in two parts [range-first, intersection-end), [intersection-end, range-last).
         //! where the first range is the intersection of the @p range and @p lookup
         //! and the second range is the complement of @range with regards to @p lookup
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename R2, typename F>
         auto intersection( R1&& range, R2&& lookup, F functor) -> decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            using range_type = decltype( make( std::forward< R1>( range)));
            using value_type = typename range_type::value_type;

            auto lambda = [&]( const value_type& value){ return find_if( std::forward< R2>( lookup), std::bind( functor, value, std::placeholders::_1));};
            return stable_partition( std::forward< R1>( range), lambda);

         }

         //!
         //! @returns a range from @p source with values not found in @p other
         //!
         //! @deprecated use intersection instead...
         template< typename R1, typename R2>
         auto difference( R1&& source, R2&& other) -> decltype( make( source))
         {
            return std::get< 1>( intersection( std::forward< R1>( source), std::forward< R2>( other)));
         }

         //!
         //! @returns a range from @p source with values not found in @p other
         //!
         template< typename R1, typename R2, typename F>
         auto difference( R1&& source, R2&& other, F functor) -> decltype( make( source))
         {
            return std::get< 1>( intersection( std::forward< R1>( source), std::forward< R2>( other), functor));
         }


         template< typename R, typename F>
         auto max( R&& range, F functor) -> decltype( make( range))
         {
            auto result = make( std::forward< R>( range));

            result.first = std::max_element( std::begin( result), std::end( result), functor);

            return result;
         }

         //!
         //! @return true if all elements in @p other is found in @p source
         //!
         template< typename R1, typename R2>
         bool includes( R1&& source, R2&& other)
         {
            using range_type = decltype( make( std::forward< R2>( other)));
            using value_type = typename range_type::value_type;

            auto lambda = [&]( const value_type& value){ return find( std::forward< R1>( source), value);};
            return all_of( other, lambda);
         }

         //!
         //! Uses @p compare to compare for equality
         //!
         //! @return true if all elements in @p other is found in @p source
         //!
         template< typename R1, typename R2, typename Compare>
         bool includes( R1&& source, R2&& other, Compare compare)
         {
            using range_type = decltype( make( std::forward< R2>( other)));
            using value_type = typename range_type::value_type;

            auto lambda = [&]( const value_type& value){ return find_if( source, std::bind( compare, std::placeholders::_1, value));};
            return all_of( other, lambda);

         }

         //!
         //! @return true if all elements in the range compare equal
         //!
         template< typename R>
         bool uniform( R&& range)
         {
            auto first = std::begin( range);

            for( auto&& value : range)
            {
               if( value != *first)
               {
                  return false;
               }
            }
            return true;
         }

         //!
         //! @return true if @p range1 includes @p range2, AND @p range2 includes @p range1
         //!
         template< typename R1, typename R2, typename Compare>
         bool uniform( R1&& range1, R2&& range2, Compare comp)
         {
            return includes( std::forward< R1>( range1), std::forward< R2>( range2), comp)
                  //&& includes( std::forward< R2>( range2), std::forward< R1>( range1), comp);
                 && includes( std::forward< R2>( range2), std::forward< R1>( range1), compare::inverse( comp));
         }



         namespace sorted
         {

            template< typename R, typename T>
            bool search( R&& range, T&& value)
            {
               return std::binary_search( std::begin( range), std::end( range), std::forward< T>( value));
            }

            template< typename R1, typename R2, typename Output>
            auto intersection( R1&& source, R2&& other, Output& result) -> decltype( make( result))
            {
               std::set_intersection(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result));

               return make( result);
            }

            template< typename R1, typename R2, typename Output, typename Compare>
            auto intersection( R1&& source, R2&& other, Output& result, Compare compare) -> decltype( make( result))
            {
               std::set_intersection(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result),
                     compare);

               return make( result);
            }

            template< typename R1, typename R2, typename Output>
            auto difference( R1&& source, R2&& other, Output& result) -> decltype( make( result))
            {
               std::set_difference(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result));

               return make( result);
            }

            template< typename R1, typename R2, typename Output, typename Compare>
            auto difference( R1&& source, R2&& other, Output& result, Compare compare) -> decltype( make( result))
            {
               std::set_difference(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result),
                     compare);

               return make( result);
            }


            template< typename R, typename T, typename C>
            auto bound( R&& range, const T& value, C compare) -> decltype( make( std::forward< R>( range)))
            {
               auto resultRange = make( std::forward< R>( range));
               resultRange.first = std::lower_bound( resultRange.first, resultRange.last, value, compare);
               resultRange.last = std::upper_bound( resultRange.first, resultRange.last, value, compare);

               return resultRange;
            }


            template< typename R, typename T>
            auto bound( R&& range, const T& value) -> decltype( make( range))
            {
               return bound( std::forward< R>( range), value, std::less< T>{});
            }


            template< typename R, typename C>
            auto group( R&& range, C compare) -> std::vector< decltype( make( std::forward< R>( range)))>
            {
               auto localRange = make( range);

               std::vector< decltype( make( std::forward< R>( range)))> result;

               while( ! localRange.empty())
               {
                  result.push_back( bound( localRange, *localRange, compare));
                  localRange.first = result.back().last;
               }

               return result;
            }

            template< typename R, typename T>
            auto group( R&& range) -> std::vector< decltype( make( std::forward< R>( range)))>
            {
               return group( std::forward< R>( range), std::less< typename R::value_type>{});
            }

         } // sorted
      } // range


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
