//!
//! casual
//!

#ifndef CASUAL_COMMON_ALGORITHM_H_
#define CASUAL_COMMON_ALGORITHM_H_


#include "common/range.h"
#include "common/traits.h"
#include "common/platform.h"
#include "common/cast.h"
#include "common/optional.h"

#include <algorithm>
#include <numeric>
#include <type_traits>
#include <ostream>


#include <functional>
#include <memory>

#include <cassert>
#include <cstring>


namespace casual
{
   namespace common
   {

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
               decltype( auto) operator() ( Values&& ...values) const
               {
                  return link_type::link( left, right, std::forward< Values>( values)...);
               }

               template< typename ...Values>
               decltype( auto) operator() ( Values&& ...values)
               {
                  return link_type::link( left, right, std::forward< Values>( values)...);
               }

            private:
               T1 left;
               T2 right;
            };

            template< typename Link, typename Arg>
            auto make( Arg&& param)
            {
               return param;
            }

            template< typename L, typename Arg, typename... Args>
            decltype( auto) make( Arg&& param, Args&&... params)
            {
               using nested_type = decltype( make< L>( std::forward< Args>( params)...));
               return basic_link< L, Arg, nested_type>( std::forward< Arg>( param), make< L>( std::forward< Args>( params)...));
            }

            struct Nested
            {
               template< typename T1, typename T2, typename T>
               static decltype( auto) link( T1& left, T2& right, T&& value)
               {
                  return left( right( value));
               }
            };

            struct And
            {
               template< typename T1, typename T2, typename T>
               static decltype( auto) link( T1& left, T2& right, T&& value)
               {
                  return left( value) && right( value);
               }
            };

            struct Or
            {
               template< typename T1, typename T2, typename T>
               static decltype( auto) link( T1& left, T2& right, T&& value)
               {
                  return left( value) || right( value);
               }
            };

            struct Order
            {
               template< typename T1, typename T2, typename V1, typename V2>
               static decltype( auto) link( T1& left, T2& right, V1&& lhs, V2&& rhs)
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
            static decltype( auto) link( Args&&... params)
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
         auto inverse( T&& functor)
         {
            return std::bind( std::forward< T>( functor), std::placeholders::_2, std::placeholders::_1);
         }


         template< typename T>
         struct Inverse
         {
            template< typename ...Args>
            Inverse( Args&&... args) : m_functor( std::forward< Args>( args)...) {}

            template< typename V1, typename V2>
            auto operator () ( V1&& lhs, V2&& rhs) const
            {
               return m_functor( std::forward< V2>( rhs), std::forward< V1>( lhs));
            }

            template< typename V1, typename V2>
            auto operator () ( V1&& lhs, V2&& rhs)
            {
               return m_functor( std::forward< V2>( rhs), std::forward< V1>( lhs));
            }

         private:
            T m_functor;
         };

      } // compare


      namespace detail
      {
         namespace coalesce
         {
            template< typename T>
            bool empty( T& value) { return value.empty();}

            template< typename T>
            bool empty( T* value) { return value == nullptr;}

            template< typename T>
            bool empty( optional< T>& value) { return ! value.has_value();}

            template< typename T>
            decltype( auto) implementation( T&& value)
            {
               return std::forward< T>( value);
            }

            template< typename T, typename... Args>
            auto implementation( T&& value, Args&&... args) ->
               std::conditional_t<
                  traits::is_same< T, Args...>::value,
                  T, // only if T and Args are exactly the same, we use T, otherwise we convert to common type
                  std::common_type_t< T, Args...>>
            {
               if( empty( value)) { return implementation( std::forward< Args>( args)...);}
               return std::forward< T>( value);
            }

         } // coalesce

      } // detail


      //!
      //! Chooses the first argument that is not 'empty'
      //!
      //! @note the return type will be the common type of all types
      //!
      //! @return the first argument that is not 'empty'
      //!
      template< typename T, typename... Args>
      decltype( auto) coalesce( T&& value,  Args&&... args)
      {
         return detail::coalesce::implementation( std::forward< T>( value), std::forward< Args>( args)...);
      }


      namespace detail
      {
         struct dummy {};

         template< typename T>
         struct negate
         {

            template< typename F>
            negate( F&& functor, dummy) : m_functor( std::forward< F>( functor))
            {

            }

            template< typename... Args>
            decltype( auto) operator () ( Args&& ...args) const
            {
               return ! m_functor( std::forward< Args>( args)...);
            }

            template< typename... Args>
            decltype( auto) operator () ( Args&& ...args)
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
         return detail::negate< T>{ std::forward< T>( functor), detail::dummy{}};
      }


      //!
      //! This is not intended to be a serious attempt at a range-library
      //! Rather an abstraction that helps our use-cases and to get a feel for
      //! what a real range-library could offer. It's a work in progress
      //!
      namespace range
      {

         template< typename R, typename = std::enable_if_t< common::traits::is::iterable< R>::value>>
         decltype( auto) reverse( R&& range)
         {
            std::reverse( std::begin( range), std::end( range));
            return std::forward< R>( range);
         }


         template< typename R, typename C, typename = std::enable_if_t< common::traits::is::iterable< R>::value>>
         decltype( auto) sort( R&& range, C compare)
         {
            std::sort( std::begin( range), std::end( range), compare);
            return std::forward< R>( range);
         }

         template< typename R, typename = std::enable_if_t< common::traits::is::iterable< R>::value>>
         decltype( auto) sort( R&& range)
         {
            std::sort( std::begin( range), std::end( range));
            return std::forward< R>( range);
         }

         template< typename R, typename C, typename = std::enable_if_t< common::traits::is::iterable< R>::value>>
         decltype( auto) stable_sort( R&& range, C compare)
         {
            std::stable_sort( std::begin( range), std::end( range), compare);
            return std::forward< R>( range);
         }

         template< typename R, typename = std::enable_if_t< common::traits::is::iterable< R>::value>>
         decltype( auto) stable_sort( R&& range)
         {
            std::stable_sort( std::begin( range), std::end( range));
            return std::forward< R>( range);
         }

         template< typename R, typename P, typename = std::enable_if_t< common::traits::is::iterable< R>::value>>
         auto partition( R&& range, P predicate)
         {
            auto middle = std::partition( std::begin( range), std::end( range), predicate);
            return std::make_tuple( make( std::begin( range), middle), make( middle, std::end( range)));
         }


         template< typename R, typename P, typename = std::enable_if_t< common::traits::is::iterable< R>::value>>
         auto stable_partition( R&& range, P predicate)
         {
            auto middle = std::stable_partition( std::begin( range), std::end( range), predicate);
            return std::make_tuple( make( std::begin( range), middle), make( middle, std::end( range)));
         }


         template< typename R, typename OutIter, typename = std::enable_if_t< 
            common::traits::iterator::is_output< OutIter>::value
            && common::traits::is::iterable< R>::value>>
         auto copy( R&& range, OutIter output)
         {
            return std::copy( std::begin( range), std::end( range), output);
         }

         template< typename R, typename Out, typename = std::enable_if_t< 
            common::traits::has::push_back< Out>::value
            && common::traits::is::iterable< R>::value>>
         decltype( auto) copy( R&& range, Out&& output)
         {
            std::copy( std::begin( range), std::end( range), std::back_inserter( output));
            return std::forward< Out>( output);
         }

         template< typename R, typename OutIter, typename P, typename = std::enable_if_t< 
            common::traits::iterator::is_output< OutIter>::value
            && common::traits::is::iterable< R>::value>>
         auto copy_if( R&& range, OutIter output, P predicate)
         {
            return std::copy_if( std::begin( range), std::end( range), output, predicate);
         }

         template< typename Range, typename Container, typename = std::enable_if_t< 
            common::traits::has::insert< Container>::value
            && common::traits::is::iterable< Range>::value>>
         Container& append( Range&& source, Container& destination)
         {
            destination.insert( std::end( destination), std::begin( source), std::end( source));
            return destination;
         }




         //!
         //! Copies from @p source to @p destination
         //!   size of destination dictates the maximum that will be
         //!   copied
         //!
         //! @param source
         //! @param destination sets the maximum what will be copied
         //!
         template< typename Range1, typename Range2>
         void copy_max( Range1&& source, Range2&& destination)
         {
            auto max = range::size( destination);
            auto wanted = range::size( source);

            if( wanted <= max)
            {
               copy( source, destination);
            }
            else
            {
               std::copy_n( std::begin( source), max, std::begin( destination));
            }
         }

         template< typename R, typename Iter>
         void copy_max( R&& range, platform::size::type size, Iter output)
         {
            if( range::size( range) <= size)
            {
               copy( range, output);
            }
            else
            {
               std::copy_n( std::begin( range), size, output);
            }
         }

         template< typename R, typename C>
         decltype( auto) move( R&& range, C& container)
         {
            std::move( std::begin( range), std::end( range), std::back_inserter( container));
            return container;
         }

         template< typename R, typename C, typename P>
         decltype( auto) move_if( R&& range, C& container, P predicate)
         {
            for( auto&& value : range)
            {
               if( predicate( value))
               {
                  container.push_back( std::move( value));
               }
            }
            return container;
         }

         namespace detail
         {
            template< typename R, typename C, typename T>
            auto transform( R&& range, C& container, T transform, category::container)
            {
               container.reserve( range::size( range) + container.size());
               std::transform( std::begin( range), std::end( range), std::back_inserter( container), transform);
               return make( std::end( container) - range::size( range), std::end( container));
            }

            template< typename R, typename O, typename T>
            decltype( auto) transform( R&& range, O&& output, T transform, category::fixed)
            {
               assert( range::size( output) >= range::size( range));
               std::transform( std::begin( range), std::end( range), std::begin( output), transform);
               return std::forward< O>( output);
            }

            template< typename R, typename O, typename T>
            auto transform( R&& range, O&& output, T transform, category::output_iterator)
            {
               auto last = std::transform( std::begin( range), std::end( range), output, transform);
               return range::make( last - range::size( range), last);;
            }
         }

         //!
         //! Transform @p range to @p container, using @p transform
         //!
         //! @param range source range/container
         //! @param container output/holder container
         //!
         //! @return range containing the inserted transformed values, previous values in @p container is excluded.
         //!
         template< typename R, typename O, typename T>
         decltype( auto) transform( R&& range, O&& output, T&& transform)
         {
            return detail::transform( std::forward< R>( range), std::forward< O>( output), std::forward< T>( transform), category::tag_t< O>{} );
         }

         //!
         //! Transform @p range, using @p transform
         //!
         //! @param range source range/container
         //!
         //! @return std::vector with the transformed values
         //!
         template< typename R, typename T>
         auto transform( R&& range, T transformer)
         {
            using value_type = std::remove_const_t< std::remove_reference_t< decltype( transformer( *std::begin( range)))>>;
            std::vector< value_type> result;

            result.reserve( range::size( range));
            std::transform( std::begin( range), std::end( range), std::back_inserter( result), transformer);

            return result;
         }


         //!
         //! Transform @p range into @p container, using @p transform if @p predicate is true
         //!
         //! @return container
         //!
         template< typename R, typename C, typename T, typename P>
         C& transform_if( R&& range, C& container, T transformer, P predicate)
         {
            for( auto&& value : range)
            {
               if( predicate( value))
               {
                  container.push_back( transformer( value));
               }
            }
            return container;
         }

         template< typename R, typename T, typename P>
         auto transform_if( R&& range, T transformer, P predicate)
         {
            std::vector< std::remove_reference_t< decltype( transformer( *std::begin( range)))>> result;
            transform_if( range, result, transformer, predicate);
            return result;
         }


         //!
         //! Applies std::unique on [std::begin( range), std::end( range) )
         //!
         //! @return the unique range
         //!
         template< typename R>
         auto unique( R&& range)
         {
            return make( std::begin( range), std::unique( std::begin( range), std::end( range)));
         }

         //!
         //! Trims @p container so it matches @p range
         //!
         //! @return range that matches the trimmed @p container
         //!
         template< typename C, typename R>
         C& trim( C& container, R&& range)
         {
            auto index = std::begin( range) - std::begin( container);
            container.erase( std::end( range), std::end( container));
            container.erase( std::begin( container), std::begin( container) + index);
            return container;
         }


         template< typename C, typename Iter>
         C& erase( C& container, Range< Iter> range)
         {
            container.erase( std::begin( range), std::end( range));
            return container;
         }

         //!
         //! Erases occurrences from an associative container that
         //! fulfill the predicate
         //!
         //! @param container associative
         //! @param predicate that takes C::mapped_type as parameter and returns bool
         //! @return the container
         //!
         template< typename C, typename P>
         C& erase_if( C& container, P&& predicate)
         {
            for( auto current = std::begin( container); current != std::end( container);)
            {
               if( predicate( current->second))
               {
                  current = container.erase( current);
               }
               else
               {
                  ++current;
               }
            }
            return container;
         }

         namespace detail
         {
            template< typename R, typename T>
            using iterator_value_compare = decltype( *std::begin( std::declval< R&>()) == std::declval< T&>());

            template< typename R1, typename R2>
            using iterator_iterator_swap = decltype( std::swap( *std::begin( std::declval< R1&>()), *std::begin( std::declval< R2&>())));
         }

         template< typename R, typename T, std::enable_if_t< common::traits::detect::is_detected< detail::iterator_value_compare, R, T>::value>* dummy = nullptr>
         auto remove( R&& range, const T& value)
         {
            return make( std::begin( range), std::remove( std::begin( range), std::end( range), value));
         }

         //!
         //! Removes the unwanted range from the source
         //! 
         template< typename R1, typename R2, std::enable_if_t< common::traits::detect::is_detected< detail::iterator_iterator_swap, R1, R2>::value>* dummy = nullptr>
         auto remove( R1&& source, R2&& unwanted)
         {
            auto last = std::rotate( std::begin( unwanted), std::end( unwanted), std::end( source));
            
            return make( std::begin( source), last);
         }

         template< typename R, typename P>
         auto remove_if( R&& range, P predicate)
         {
            return make( std::begin( range), std::remove_if( std::begin( range), std::end( range), predicate));
         }


         template< typename R1, typename R2, typename P>
         bool equal( R1&& lhs, R2&& rhs, P predicate)
         {
            return std::equal( std::begin( lhs), std::end( lhs), 
               std::begin( rhs), std::end( rhs), predicate);
         }


         template< typename R1, typename R2>
         bool equal( R1&& lhs, R2&& rhs)
         {
            return std::equal( std::begin( lhs), std::end( lhs), 
               std::begin( rhs), std::end( rhs));
         }


         template< typename R, typename T>
         decltype( auto) accumulate( R&& range, T&& value)
         {
            return std::accumulate( std::begin( range), std::end( range), std::forward< T>( value));
         }

         template< typename R, typename T, typename F>
         decltype( auto) accumulate( R&& range, T&& value, F&& functor)
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

         template< typename R, typename P>
         bool none_of( R&& range, P predicate)
         {
            return std::none_of( std::begin( range), std::end( range), predicate);
         }


         template< typename R, typename F>
         decltype( auto) for_each( R&& range, F functor)
         {
            std::for_each( std::begin( range), std::end( range), functor);
            return std::forward< R>( range);
         }

         template< typename R, typename N, typename F>
         auto for_each_n( R&& range, N n, F functor) -> decltype( range::make( std::forward< R>( range)))
         {
            if( range::size( range) <= n)
            {
               return for_each( std::forward< R>( range), functor);
            }
            else
            {
               return for_each( range::make( std::begin( range), n), functor);
            }
         }


         //!
         //! associate container specialization
         //!
         template< typename R, typename T,
            std::enable_if_t< common::traits::container::is_associative< std::decay_t< R>>::value>* dummy = nullptr>
         auto find( R&& range, T&& value)
         {
            return make( range.find( value), std::end( range));
         }

         //!
         //! non associate container specialization
         //!
         template< typename R, typename T,
            std::enable_if_t< ! common::traits::container::is_associative< std::decay_t< R>>::value>* dummy = nullptr>
         auto find( R&& range, T&& value)
         {
            return make( std::find( std::begin( range), std::end( range), std::forward< T>( value)), std::end( range));
         }



         template< typename R, typename P>
         auto find_if( R&& range, P predicate)
         {
            return make( std::find_if( std::begin( range), std::end( range), predicate), std::end( range));
         }


         template< typename R>
         auto adjacent_find( R&& range)
         {
            return make( std::adjacent_find( std::begin( range), std::end( range)), std::end( range));
         }


         template< typename R, typename P>
         auto adjacent_find( R&& range, P predicate)
         {
            return make( std::adjacent_find( std::begin( range), std::end( range), predicate), std::end( range));
         }


         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence that is equal to @p value
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename T>
         auto divide( R1&& range, T&& value)
         {
            auto divider = std::find(
                  std::begin( range), std::end( range),
                  std::forward< T>( value));

            return std::make_tuple( make( std::begin( range), divider), make( divider, std::end( range)));
         }

         //!
         //! Split @p range in two parts [range-first, divider), [divider + 1, range-last).
         //! where divider is the first occurrence of @p value
         //!
         //! That is, exactly as divide but the divider is omitted in the resulting ranges
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R, typename T>
         auto split( R&& range, T&& value)
         {
            auto result = divide( std::forward< R>( range), std::forward< T>( value));
            if( ! std::get< 1>( result).empty())
            {
               ++std::get< 1>( result);
            }
            return result;
         }

         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence where @p predicate is true
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename P>
         auto divide_if( R1&& range, P predicate)
         {
            auto divider = std::find_if( std::begin( range), std::end( range), predicate);

            return std::make_tuple( make( std::begin( range), divider), make( divider, std::end( range)));
         }
         

         template< typename R1, typename R2>
         auto search( R1&& range, R2&& to_find)
         {
            auto first = std::search( std::begin( range), std::end( range), std::begin( to_find), std::end( to_find));
            return make( first, std::end( range));
         }


         template< typename R1, typename R2, typename F>
         auto find_first_of( R1&& target, R2&& source, F functor)
         {
            auto result = make( std::forward< R1>( target));

            result.first = std::find_first_of(
                  std::begin( result), std::end( result),
                  std::begin( source), std::end( source),
                  functor);

            return result;
         }


         template< typename R1, typename R2>
         auto find_first_of( R1&& target, R2&& source)
         {
            auto result = make( std::forward< R1>( target));

            result.first = std::find_first_of(
                  std::begin( result), std::end( result),
                  std::begin( source), std::end( source));

            return result;
         }

         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence found in @p lookup
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename R2>
         auto divide_first( R1&& range, R2&& lookup)
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
         auto divide_first( R1&& range, R2&& lookup, F functor)
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
         auto intersection( R1&& range, R2&& lookup)
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
         auto intersection( R1&& range, R2&& lookup, F functor)
         {
            auto lambda = [&]( auto v){
               return find_if( std::forward< R2>( lookup), [&]( auto& l){
                  return functor( v, l);
               });
            };
            return stable_partition( std::forward< R1>( range), lambda);

         }

         //!
         //! @returns a range from @p source with values not found in @p other
         //!
         //! @deprecated use intersection instead...
         template< typename R1, typename R2>
         auto difference( R1&& source, R2&& other)
         {
            return std::get< 1>( intersection( std::forward< R1>( source), std::forward< R2>( other)));
         }

         //!
         //! @returns a range from @p source with values not found in @p other
         //!
         template< typename R1, typename R2, typename F>
         auto difference( R1&& source, R2&& other, F functor)
         {
            return std::get< 1>( intersection( std::forward< R1>( source), std::forward< R2>( other), functor));
         }


         template< typename R, typename F>
         auto max( R&& range, F functor)
         {
            //
            // Just to make sure range is not an rvalue container. we could use enable_if instead
            //
            auto result = make( std::forward< R>( range));

            return make( std::max_element( std::begin( result), std::end( result), functor), std::end( result));
         }

         template< typename R>
         auto max( R&& range)
         {
            //
            // Just to make sure range is not an rvalue container. we could use enable_if instead
            //
            auto result = make( std::forward< R>( range));

            return make( std::max_element( std::begin( result), std::end( result)), std::end( result));
         }

         template< typename R, typename F>
         auto min( R&& range, F functor)
         {
            //
            // Just to make sure range is not an rvalue container. we could use enable_if instead.
            //
            auto result = make( std::forward< R>( range));

            return make( std::min_element( std::begin( result), std::end( result), functor), std::end( result));
         }

         template< typename R>
         auto min( R&& range)
         {
            //
            // Just to make sure range is not an rvalue container. we could use enable_if instead.
            //
            auto result = make( std::forward< R>( range));

            return make( std::min_element( std::begin( result), std::end( result)), std::end( result));
         }


         //!
         //! @return true if all elements in @p other is found in @p source
         //!
         template< typename R1, typename R2>
         bool includes( R1&& source, R2&& other)
         {
            auto lambda = [&]( const auto& value){ return find( std::forward< R1>( source), value);};
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
            auto lambda = [&]( const auto& v)
                  { return find_if( source, [&]( const auto& s){ return compare( s, v);});};
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
                 && includes( std::forward< R2>( range2), std::forward< R1>( range1), compare::inverse( comp));
         }


         template< typename Range, typename Predicate>
         auto count_if( Range&& range, Predicate predicate)
         {
            return std::count_if( std::begin( range), std::end( range), predicate);
         }


         template< typename Stream, typename Range, typename D>
         Stream& print( Stream& out, Range&& range, D&& delimeter)
         {
            auto values = range::make( range);

            if( values.empty())
               return out;

            out << *values;
            ++values;

            for( auto& value : values)
            {
               out << delimeter << value;
            }
            return out;
         }

         template< typename Stream, typename Range, typename D, typename F>
         Stream& print( Stream& out, Range&& range,  D&& delimeter, F&& functor)
         {
            auto values = range::make( range);

            if( values.empty())
               return out;

            functor( out, *values);
            ++values;

            for( auto& value : values)
            {
               out << delimeter;
               functor( out, value);
            }
            return out;
         }
      

         namespace numeric
         {
            template< typename R, typename T>
            void iota( R&& range, T value)
            {
               std::iota( std::begin( range), std::end( range), value);
            }

         } // numeric


         namespace sorted
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

            template< typename R1, typename R2, typename Output>
            auto intersection( R1&& source, R2&& other, Output& result)
            {
               std::set_intersection(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result));

               return make( result);
            }

            template< typename R1, typename R2, typename Output, typename Compare>
            auto intersection( R1&& source, R2&& other, Output& result, Compare compare)
            {
               std::set_intersection(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result),
                     compare);

               return make( result);
            }

            template< typename R1, typename R2, typename Output>
            auto difference( R1&& source, R2&& other, Output& result)
            {
               std::set_difference(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result));

               return make( result);
            }

            template< typename R1, typename R2, typename Output, typename Compare>
            auto difference( R1&& source, R2&& other, Output& result, Compare compare)
            {
               std::set_difference(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result),
                     compare);

               return make( result);
            }


            template< typename R, typename T, typename C>
            auto bound( R&& range, const T& value, C compare)
            {
               auto first = std::lower_bound( std::begin( range), std::end( range), value, compare);
               auto last = std::upper_bound( first, std::end( range), value, compare);

               return make( first, last);
            }


            template< typename R, typename T>
            auto bound( R&& range, const T& value)
            {
               return bound( std::forward< R>( range), value, std::less< T>{});
            }


            template< typename R, typename C>
            auto group( R&& range, C compare)
            {
               std::vector< range::type_t< R>> result;

               auto current = make( range);

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

            //!
            //! Returns a subrange that consists of (the first series of) values that fulfills the predicate
            //!
            //! @param range
            //! @param predicate
            //! @return
            template< typename R, typename P>
            auto subrange( R&& range, P&& predicate)
            {
               auto first = std::find_if( std::begin( range), std::end( range), predicate);
               return range::make( first, std::find_if( first, std::end( range), negate( predicate)));
            }

         } // sorted
      } // range

      template< typename Iter>
      std::ostream& operator << ( std::ostream& out, Range< Iter> range)
      {
         if( out)
         {
            out << "[";
            range::print( out, range, ',') << ']';
         }
         return out;
      }

      template< typename Iter1, typename Iter2>
      bool operator == ( const Range< Iter1>& lhs, const Range< Iter2>& rhs)
      {
         return range::equal( lhs, rhs);
      }

      template< typename Iter, typename C,
         std::enable_if_t< common::traits::container::is_container< std::remove_reference_t< C>>::value>* dummy = nullptr>
      bool operator == ( const Range< Iter>& lhs, const C& rhs)
      {
         return range::equal( lhs, rhs);
      }

      template< typename C, typename Iter,
         std::enable_if_t< common::traits::container::is_container< std::remove_reference_t< C>>::value>* dummy = nullptr>
      bool operator == ( C& lhs, const Range< Iter>& rhs)
      {
         return range::equal( lhs, rhs);
      }

      template< typename Iter>
      bool operator == ( const Range< Iter>& lhs, bool rhs)
      {
         return static_cast< bool>( lhs) ==  rhs;
      }


      namespace compare
      {
         template< typename T, typename R>
         bool any( T&& value, R&& range)
         {
            return range::find( range, value);
         }

         template< typename T, typename V>
         bool any( T&& value, std::initializer_list< V> range)
         {
            return range::find( range, value);
         }
      } // compare


   } // common
} // casual

namespace std
{
   template< typename Enum>
   enable_if_t< is_enum< Enum>::value, ostream&>
   operator << ( ostream& out, Enum value)
   {
     return out << casual::common::cast::underlying( value);
   }

} // std




#endif // CASUAL_COMMON_ALGORITHM_H_
