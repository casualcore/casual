//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/range.h"
#include "common/traits.h"
#include "casual/platform.h"
#include "common/predicate.h"

#include <algorithm>
#include <numeric>
#include <optional>
#include <type_traits>

#include <ostream>


#include <functional>
#include <memory>

#include <cassert>
#include <cstring>


namespace casual
{
   //! This is not intended to be a serious attempt at a range-library
   //! Rather an abstraction that helps our use-cases and to get a feel for
   //! what a real range-library could offer. It's a work in progress
   namespace common::algorithm
   {

      template< concepts::range R>
      decltype( auto) reverse( R&& range)
      {
         std::reverse( std::begin( range), std::end( range));
         return std::forward< R>( range);
      }

      namespace detail
      {
         template< typename P>
         auto pivot( P& pivot)
         {
            if constexpr( std::contiguous_iterator< P>)
               return pivot;
            else
               return std::begin( pivot);
         }

      } // detail

      //! Rotates the range based on the pivot.
      //! @return a tuple of { [begin( range), pivot), pivot, end( range)]}
      template< concepts::range R, typename P>
      auto rotate( R&& range, P&& pivot)
      {
         auto middle = std::rotate( std::begin( range), detail::pivot( pivot), std::end( range));
         return std::make_tuple( range::make( std::begin( range), middle), range::make( middle, std::end( range)));
      }

      template< concepts::range R, typename C>
      auto sort( R&& range, C compare) 
         -> decltype( void( std::sort( std::begin( range), std::end( range), compare)), std::forward< R>( range))
      {
         std::sort( std::begin( range), std::end( range), compare);
         return std::forward< R>( range);
      }

      template< concepts::range R>
      auto sort( R&& range) 
         -> decltype( void( std::sort( std::begin( range), std::end( range))), std::forward< R>( range))
      {
         std::sort( std::begin( range), std::end( range));
         return std::forward< R>( range);
      }

      template< concepts::range R, typename C>
      auto stable_sort( R&& range, C compare)
         -> decltype( void( std::stable_sort( std::begin( range), std::end( range), compare)), std::forward< R>( range))
      {
         std::stable_sort( std::begin( range), std::end( range), compare);
         return std::forward< R>( range);
      }

      template< concepts::range R>
      auto stable_sort( R&& range) 
         -> decltype( void( std::stable_sort( std::begin( range), std::end( range))), std::forward< R>( range))
      {
         std::stable_sort( std::begin( range), std::end( range));
         return std::forward< R>( range);
      }

      //! fills the container with `value`
      //! @returns the range
      template< concepts::range R, typename V>
      decltype( auto) fill( R&& range, V&& value)
      {
         std::fill( std::begin( range), std::end( range), std::forward< V>( value));
         return std::forward< R>( range);
      }

      namespace detail
      {

         namespace output
         {
            template< concepts::container::back_inserter C> 
            auto iterator( C& value, traits::priority::tag< 2>)
            {
               return std::back_inserter( value);
            }

            template< typename C> 
            auto iterator( C& value, traits::priority::tag< 1>)
               -> decltype( std::begin( value))
            {
               return std::begin( value);
            }

            template< typename I> 
            auto iterator( I&& value, traits::priority::tag< 0>)
               -> decltype( void( *value++ = *value), std::forward< I>( value))
            {
               return std::forward< I>( value);
            }

            template< typename C>
            auto iterator( C&& value)
               -> decltype( iterator( std::forward< C>( value), traits::priority::tag< 2>{}))
            {
               return iterator( std::forward< C>( value), traits::priority::tag< 2>{});
            }
         } // output

         template< typename R, typename Out> 
         auto copy( R&& range, Out&& output, traits::priority::tag< 0>) 
            -> decltype( std::copy( std::begin( range), std::end( range), output))
         {
            return std::copy( std::begin( range), std::end( range), output);
         }

         template< typename R, typename Out> 
         auto copy( R&& range, Out&& output, traits::priority::tag< 1>) 
            -> decltype( std::copy( std::begin( range), std::end( range), std::begin( output)), std::forward< Out>( output))
         {
            std::copy( std::begin( range), std::end( range), std::begin( output));
            return std::forward< Out>( output);
         }

         template< typename R, typename Out> 
         auto copy( R&& range, Out&& output, traits::priority::tag< 2>) 
            -> decltype( output.resize( std::distance( std::begin( range), std::end( range))), std::forward< Out>( output))
         {
            output.resize( std::distance( std::begin( range), std::end( range)));
            std::copy( std::begin( range), std::end( range), std::begin( output));
            return std::forward< Out>( output);
         }
         

      } // detail

      //! copy `range` to `output`
      //!
      //! @attention the possible content in `output` is truncated and overwritten
      //! 
      //! @return output
      template< concepts::range R, typename Out> 
      auto copy( R&& range, Out&& output) -> decltype( detail::copy( std::forward< R>( range), std::forward< Out>( output), traits::priority::tag< 2>{}))
      {
         return detail::copy( std::forward< R>( range), std::forward< Out>( output), traits::priority::tag< 2>{});
      }



      template< concepts::range R, typename Out, typename P> 
      auto copy_if( R&& range, Out&& output, P predicate)
      {
         return std::copy_if( std::begin( range), std::end( range), detail::output::iterator( std::forward< Out>( output)), predicate);
      }

   

      template< concepts::range R, typename Iter>
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

      //! Copies from @p source to @p destination
      //!   size of destination dictates the maximum that will be
      //!   copied
      //!
      //! @param source
      //! @param destination sets the maximum what will be copied
      template< concepts::range Range1, concepts::range Range2>
      void copy_max( Range1&& source, Range2&& destination)
      {
         copy_max( source, range::size( destination), std::begin( destination));
      }

      //! move range to out.
      //! if out has push_back it will be used, hance append.
      template< concepts::range R, typename Out>
      decltype( auto) move( R&& range, Out&& out)
      {
         std::move( std::begin( range), std::end( range), detail::output::iterator( out));
         return std::forward< Out>( out);
      }

      template< concepts::range R, typename C, typename P>
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
         template< typename R, concepts::container::sequence C, typename T>
         auto transform( R&& range, C& container, T transform)
         {
            container.reserve( range::size( range) + container.size());
            std::transform( std::begin( range), std::end( range), std::back_inserter( container), transform);
            return range::make( std::end( container) - range::size( range), std::end( container));
         }

         template< typename R, concepts::container::associative C, typename T>
         decltype( auto) transform( R&& range, C&& container, T transform)
         {
            container.reserve( range::size( range) + container.size());
            for( auto& value: range)
               container.emplace( transform( value));

            return std::forward< C>( container);
         }

         template< typename R, concepts::container::array O, typename T>
         decltype( auto) transform( R&& range, O& output, T transform)
         {
            assert( range::size( output) >= range::size( range));
            std::transform( std::begin( range), std::end( range), std::begin( output), transform);
            return std::forward< O>( output);
         }

         template< typename R, std::input_or_output_iterator O, typename T>
         O transform( R&& range, O output, T transform)
         {
            std::transform( std::begin( range), std::end( range), output, transform);
            return output;
         }
      } // detail

      //! Transform @p range to @p container, using @p transform
      //!
      //! @param range source range/container
      //! @param container output/holder container
      //!
      //! @return range containing the inserted transformed values, previous values in @p container is excluded.
      template< concepts::range R, typename O, typename T>
      decltype( auto) transform( R&& range, O&& output, T&& transform)
      {
         return detail::transform( std::forward< R>( range), std::forward< O>( output), std::forward< T>( transform));
      }

      //! Transform @p range, using @p transform
      //!
      //! @param range source range/container
      //!
      //! @return std::vector with the transformed values
      template< concepts::range R, typename T>
      [[nodiscard]] auto transform( R&& range, T transformer)
      {
         using value_type = std::remove_const_t< std::remove_reference_t< decltype( transformer( *std::begin( range)))>>;
         std::vector< value_type> result;

         result.reserve( range::size( range));
         std::transform( std::begin( range), std::end( range), std::back_inserter( result), transformer);

         return result;
      }

      //! Transform @p range into @p output, using @p transform if @p predicate is true
      //!
      //! @return container
      template< concepts::range R, typename Out, typename T, typename P>
      decltype( auto) transform_if( R&& range, Out&& output, T transformer, P predicate)
      {
         auto&& out = detail::output::iterator( output);

         for( auto&& value : range)
         {
            if( predicate( value))
               *out++ = transformer( value);
         }
         return std::forward< Out>( output);
      }

      template< concepts::range R, typename T, typename P>
      [[nodiscard]] auto transform_if( R&& range, T transformer, P predicate)
      {
         std::vector< std::remove_reference_t< decltype( transformer( *std::begin( range)))>> result;
         return transform_if( range, std::move( result), transformer, predicate);
      }

      //! @returns a 'generated' vector of size `N`
      template< platform::size::type N, typename G>
      auto generate_n( G generator) 
         -> std::vector< std::remove_reference_t< decltype( generator())>>
      {
         std::vector< std::remove_reference_t< decltype( generator())>> result;
         result.reserve( N);
         std::generate_n( std::back_inserter( result), N, std::move( generator));
         return result;
      }

      //! @returns a 'generated' vector of size `N`
      template< platform::size::type N, typename G>
      auto generate_n( G generator) 
         -> std::vector< std::remove_reference_t< decltype( generator( N))>>
      {
         std::vector< std::remove_reference_t< decltype( generator( N))>> result;

         for( auto count = ( N - N); count < N; ++count)
            result.push_back( generator( count));

         return result;
      }

      //! Applies std::unique on [std::begin( range), std::end( range) )
      //!
      //! @return the unique range
      //! @{
      template< concepts::range R>
      [[nodiscard]] auto unique( R&& range)
      {
         return range::make( std::begin( range), std::unique( std::begin( range), std::end( range)));
      }

      template< concepts::range R, typename P>
      [[nodiscard]] auto unique( R&& range, P predicate)
      {
         return range::make( std::begin( range), std::unique( std::begin( range), std::end( range), predicate));
      }
      //! @}


      namespace detail
      {
         template< typename T, typename U>
         concept range_value_compare = requires( const T& a, const U& b)
         {
            { *std::begin( a) == b} -> std::convertible_to< bool>;
         };
      }

      template< concepts::range R, typename T>
      [[nodiscard]] auto remove( R&& range, const T& value) requires detail::range_value_compare< R, T>
      {
         return range::make( std::begin( range), std::remove( std::begin( range), std::end( range), value));
      }

      //! Removes the unwanted range from the source
      template< concepts::range R1, concepts::range R2>
      [[nodiscard]] auto remove( R1&& source, R2&& unwanted)
      {
         auto last = std::rotate( std::begin( unwanted), std::end( unwanted), std::end( source));
         
         return range::make( std::begin( source), last);
      }

      template< concepts::range R, typename P>
      [[nodiscard]] auto remove_if( R&& range, P predicate)
      {
         return range::make( std::begin( range), std::remove_if( std::begin( range), std::end( range), predicate));
      }


      template< concepts::range R1, concepts::range R2, typename P>
      [[nodiscard]] bool equal( R1&& lhs, R2&& rhs, P predicate)
      {
         return std::equal( std::begin( lhs), std::end( lhs), 
            std::begin( rhs), std::end( rhs), predicate);
      }


      template< concepts::range R1, concepts::range R2>
      [[nodiscard]] bool equal( R1&& lhs, R2&& rhs)
      {
         return std::equal( std::begin( lhs), std::end( lhs), 
            std::begin( rhs), std::end( rhs));
      }


      template< concepts::range R, typename T>
      [[nodiscard]] constexpr T accumulate( R&& range, T result)
      {
         // TODO maintainence: use std::accumulate when c++20
         for( auto& value : range)
            result = std::move( result) + value;
         return result;
      }

      template< concepts::range R, typename T, typename F>
      [[nodiscard]] constexpr T accumulate( R&& range, T result, F&& functor)
      {
         // TODO maintainence: use std::accumulate when c++20
         for( auto& value : range)
            result = functor( std::move( result), value);
         return result;
      }



      template< concepts::range R, typename P>
      bool all_of( R&& range, P predicate)
      {
         return std::all_of( std::begin( range), std::end( range), predicate);
      }

      template< concepts::range R, typename P>
      bool any_of( R&& range, P predicate)
      {
         return std::any_of( std::begin( range), std::end( range), predicate);
      }

      template< concepts::range R, typename P>
      bool none_of( R&& range, P predicate)
      {
         return std::none_of( std::begin( range), std::end( range), predicate);
      }

      //! applies `functor` to all elements
      template< concepts::range R, typename F>
      decltype( auto) for_each( R&& range, F&& functor)
      {
         std::for_each( std::begin( range), std::end( range), std::forward< F>( functor));
         return std::forward< R>( range);
      }

      //! applies `functor` to all elements that `predicate` is true.
      template< concepts::range R, typename F, typename P>
      decltype( auto) for_each_if( R&& range, F&& functor, P&& predicate)
      {
         for( auto& value : range)
         {
            if( predicate( value))
               functor( value);
         }
         return std::forward< R>( range);
      }

      //! applies `functor` to all elements that `element` is true.
      template< concepts::range R, typename F>
      decltype( auto) for_each_if( R&& range, F&& functor)
      {
         for( auto& value : range)
         {
            if( value)
               functor( value);
         }
         return std::forward< R>( range);
      }

      //! applies `functor` for all elements in `range` while `functor` returns true.
      //! Hence, if `functor` returns false the invocation stops
      template< concepts::range R, typename F>
      decltype( auto) for_each_while( R&& range, F functor)
      {  
         for( auto current = range::make( range); current && functor( *current); ++current)
            ; // no-op
         
         return std::forward< R>( range);
      }

      //! applies `functor` on all elements that are equal to `value`
      template< concepts::range R, typename V, typename F>
      decltype( auto) for_each_equal( R&& range, V&& value, F functor)
      {
         for( auto&& element : range)
         {
            if( element == value)
               functor( element);
         }
         return std::forward< R>( range);
      }

      template< concepts::range R, typename F>
      auto for_each_n( R&& range, platform::size::type n, F functor) -> decltype( range::make( std::forward< R>( range)))
      {
         if( range::size( range) <= n)
            return for_each( range::make( range), functor);
         else
            return for_each( range::make( std::begin( range), n), functor);
      }

      //! applies `functor` on all occurrences, and call `interleave` between each
      template< concepts::range R, typename F, typename I>
      decltype( auto) for_each_interleave( R&& range, F&& functor, I&& interleave)
      {
         if( ! range::empty( range))
         {
            auto current = std::begin( range);
            functor( *current++);

            while( current != std::end( range))
            {
               interleave();
               functor( *current++);
            }
         }

         return std::forward< R>( range);
      }


      namespace detail
      {
         template< typename F>
         constexpr auto for_n( platform::size::type n, F functor, common::traits::priority::tag< 1>) 
            -> decltype( void( functor( n)))
         {
            for( platform::size::type index = 0; index < n; ++index)
               functor( index);
         }

         template< typename F>
         constexpr auto for_n( platform::size::type n, F functor, common::traits::priority::tag< 0>) 
            -> decltype( void( functor()))
         {
            for( platform::size::type index = 0; index < n; ++index)
               functor();
         }
      } // detail

      //! applies `functor` `N` times, from template value N.
      template< platform::size::type N, typename F>
      constexpr void for_n( F functor)
      {
         detail::for_n( N, std::move( functor), common::traits::priority::tag< 1>{});
      }

      //! applies `functor` `n` times
      template< typename F>
      constexpr auto for_n( platform::size::type n, F functor) 
         -> decltype( detail::for_n( n, std::move( functor), common::traits::priority::tag< 1>{}))
      {
         detail::for_n( n, std::move( functor), common::traits::priority::tag< 1>{});
      }


      namespace detail
      {
         //! associate container specialization
         template< typename R, typename T>
         constexpr auto find( R&& range, const T& value, traits::priority::tag< 1>) 
            -> decltype( range::make( range.find( value), std::end( range)))
         {
            return range::make( range.find( value), std::end( range));
         }

         //! non associate container specialization
         template< typename R, typename T>
         constexpr auto find( R&& range, const T& value, traits::priority::tag< 0>)
            -> decltype( range::make( std::find( std::begin( range), std::end( range), value), std::end( range)))
         {
            return range::make( std::find( std::begin( range), std::end( range), value), std::end( range));
         }
      } // detail

      //! @returns a range [found, end( range)) - empty range if not found.
      template< concepts::range R, typename T>
      constexpr auto find( R&& range, const T& value)
         -> decltype( detail::find( std::forward< R>( range), value, traits::priority::tag< 1>{}))
      {
         return detail::find( std::forward< R>( range), value, traits::priority::tag< 1>{});
      }

      template< concepts::range R, typename P>
      constexpr auto find_if( R&& range, P predicate)
      {
         return range::make( std::find_if( std::begin( range), std::end( range), predicate), std::end( range));
      }


      template< concepts::range R, typename P>
      auto adjacent_find( R&& range, P predicate)
      {
         return range::make( std::adjacent_find( std::begin( range), std::end( range), predicate), std::end( range));
      }

      template< concepts::range R>
      auto adjacent_find( R&& range)
      {
         return range::make( std::adjacent_find( std::begin( range), std::end( range)), std::end( range));
      }

      namespace detail
      {
         template< typename R, typename T>
         auto contains( R&& range, const T& value, traits::priority::tag< 2>) 
            -> decltype( range.contains( value))
         {
            return range.contains( value);
         }

         template< typename R, typename T>
         auto contains( R&& range, const T& value, traits::priority::tag< 1>) 
            -> decltype( range.find( value) != std::end( range))
         {
            return range.find( value) != std::end( range);
         }
         
         template< typename R, typename T>
         auto contains( R&& range, const T& value, traits::priority::tag< 0>) 
            -> decltype( ! algorithm::find( range, value).empty())
         {
            return ! algorithm::find( range, value).empty();
         }
      } // detail

      //! @returns true if `value` is found in `range` - false otherwise
      template< concepts::range R, typename T>
      auto contains( R&& range, const T& value)
         -> decltype( detail::contains( std::forward< R>( range), value, traits::priority::tag< 2>{}))
      {
         return detail::contains( std::forward< R>( range), value, traits::priority::tag< 2>{});
      }


      template< concepts::range R, typename V>
      auto replace( R&& range, V&& old_value, V&& new_value)
      {
         std::replace( std::begin( range), std::end( range), old_value, new_value);
         return std::forward< R>( range);
      }


      namespace detail
      {
         template< typename C, typename T>
         auto append( C& container, T&& value, traits::priority::tag< 0>) 
            -> decltype( void( container.push_back( std::forward< T>( value))))
         {
            container.push_back( std::forward< T>( value));
         }

         template< typename C, typename T>
         auto append( C& container, T&& value) 
            -> decltype( append( container, std::forward< T>( value), traits::priority::tag< 0>{}))
         {
            append( container, std::forward< T>( value), traits::priority::tag< 0>{});
         }
      } // detail

      //! push_back `value` to `output` if it's not already present
      //! @return true if push_back
      //! @{
      template< typename T, typename O, typename P> 
      auto append_unique_value( T&& value, O& output, P predicate)
         -> decltype( detail::append( output, std::forward< T>( value)), bool())
      {
         auto equal = [&value, predicate]( auto& existing){ return predicate( value, existing);};
         if( algorithm::find_if( output, equal))
            return false;
         
         detail::append( output, std::forward< T>( value));
         return true;
      }

      template< typename T, typename O> 
      auto append_unique_value( T&& value, O&& output)
         -> decltype( detail::append( output, std::forward< T>( value)), bool())
      {
         if( algorithm::find( output, value))
            return false;
         
         detail::append( output, std::forward< T>( value));
         return true;
      }
      //! @}


      template< typename T, typename O, typename P>
      auto append_replace_value( T&& value, O& output, P predicate)
         -> decltype( void( detail::append( output, std::forward< T>( value))))
      {
         auto equal = [&value, predicate]( auto& existing){ return predicate( value, existing);};
         if( auto found = algorithm::find_if( output, equal))
            *found = std::forward< T>( value);
         else
            detail::append( output, std::forward< T>( value));
      }

      template< typename T, typename O> 
      auto append_replace_value( T&& value, O& output) 
         -> decltype( void( detail::append( output, std::forward< T>( value))))
      {
         if( auto found = algorithm::find( output, value))
            *found = std::forward< T>( value);
         else
            detail::append( output, std::forward< T>( value));
      }


      template< concepts::range R, typename O, typename P> 
      auto append_unique( R& range, O&& output, P predicate)
         -> decltype( void( append_unique_value( range::front( range), output, predicate)), std::forward< O>( output))
      {
         for( auto& value : range)
            append_unique_value( value, output, predicate);

         return std::forward< O>( output);
      }

      template< concepts::range R, typename O> 
      auto append_unique( R&& range, O&& output) 
      -> decltype( void( append_unique_value( range::front( range), output)), std::forward< O>( output))
      {
         for( auto& value : range)
            append_unique_value( value, output);

         return std::forward< O>( output);
      }
      
      template< concepts::range R, typename O, typename P> 
      auto append_replace( R&& range, O&& output, P predicate)
         -> decltype( void( append_replace_value( range::front( range), output, predicate)), std::forward< O>( output))
      {
         for( auto& value : range)
            append_replace_value( value, output, predicate);

         return std::forward< O>( output);
      }

      template< concepts::range R, typename O> 
      auto append_replace( R&& range, O&& output) 
         -> decltype( void( append_replace_value( range::front( range), output)), std::forward< O>( output))
      {
         for( auto& value : range)
            append_replace_value( value, output);

         return std::forward< O>( output);
      }

      //! partition `range` based on `predicate`
      //! @returns tuple with [begin( range), partition-end) [partition-end, end( range)] 
      template< concepts::range R, typename P>
      auto partition( R&& range, P predicate)
         -> decltype( std::make_tuple( range::make( range), range::make( std::partition( std::begin( range), std::end( range), predicate), std::end( range))))
      {
         auto middle = std::partition( std::begin( range), std::end( range), predicate);
         return std::make_tuple( range::make( std::begin( range), middle), range::make( middle, std::end( range)));
      }

      //! @returns std::get< 0>( algorithm::partition( range, predicate));
      template< concepts::range R, typename P>
      auto filter( R&& range, P predicate) -> std::remove_cvref_t< decltype( std::get< 0>( partition( range, predicate)))>
      {
         return std::get< 0>( partition( range, predicate));
      }

      namespace stable
      {
         template< concepts::range R, typename P>
         auto partition( R&& range, P predicate)
         {
            auto middle = std::stable_partition( std::begin( range), std::end( range), predicate);
            return std::make_tuple( range::make( std::begin( range), middle), range::make( middle, std::end( range)));
         }

         //! @returns std::get< 0>( algorithm::stable::partition( range, predicate));
         template< concepts::range R, typename P>
         auto filter( R&& range, P predicate) -> std::remove_cvref_t< decltype( std::get< 0>( partition( range, predicate)))>
         {
            return std::get< 0>( partition( range, predicate));
         }            
      } // stable

      //! Divide @p range in two parts [range-first, divider), [divider, range-last).
      //! where divider is the first occurrence that is equal to @p value
      //!
      //! @return a tuple with the two ranges
      template< concepts::range R, typename T>
      [[nodiscard]] auto divide( R&& range, const T& value)
      {
         auto divider = std::find(
               std::begin( range), std::end( range),
               value);

         return std::make_tuple( range::make( std::begin( range), divider), range::make( divider, std::end( range)));
      }

      //! Split @p range in two parts [range-first, divider), [divider + 1, range-last).
      //! where divider is the first occurrence of @p value
      //!
      //! That is, exactly as divide but the divider is omitted in the resulting ranges
      //!
      //! @return a tuple with the two ranges
      template< concepts::range R, typename T>
      [[nodiscard]] auto split( R&& range, const T& value)
      {
         auto result = divide( std::forward< R>( range), value);
         if( ! std::get< 1>( result).empty())
            ++std::get< 1>( result);
            
         return result;
      }

      //! Divide @p range in two parts [range-first, divider), [divider, range-last).
      //! where divider is the first occurrence where @p predicate is true
      //!
      //! @return a tuple with the two ranges
      template< concepts::range R1, typename P>
      [[nodiscard]] auto divide_if( R1&& range, P predicate)
      {
         auto divider = std::find_if( std::begin( range), std::end( range), predicate);

         return std::make_tuple( range::make( std::begin( range), divider), range::make( divider, std::end( range)));
      }
      

      template< concepts::range R1, concepts::range R2>
      [[nodiscard]] auto search( R1&& range, R2&& to_find)
      {
         auto first = std::search( std::begin( range), std::end( range), std::begin( to_find), std::end( to_find));
         return range::make( first, std::end( range));
      }


      template< concepts::range R1, concepts::range R2, typename F>
      [[nodiscard]] auto find_first_of( R1&& source, R2&& lookup, F functor)
      {
         auto found = std::find_first_of(
               std::begin( source), std::end( source),
               std::begin( lookup), std::end( lookup),
               functor);

         return range::make( found, std::end( source));
      }

      template< concepts::range R1, concepts::range R2>
      [[nodiscard]] auto find_first_of( R1&& target, R2&& source)
      {
         return find_first_of( std::forward< R1>( target), std::forward< R2>( source), std::equal_to<>{});
      }

      //! Divide @p range in two parts [range-first, divider), [divider, range-last).
      //! where divider is the first occurrence found in @p lookup
      //!
      //! @return a tuple with the two ranges
      template< concepts::range R1, concepts::range R2, typename F>
      [[nodiscard]] auto divide_first( R1&& range, R2&& lookup, F functor)
      {
         auto divider =  find_first_of( range, lookup, functor);

         return std::make_tuple( range::make( std::begin( range), std::begin( divider)), divider);
      }

      //! Divide @p range in two parts [range-first, divider), [divider, range-last).
      //! where divider is the first occurrence found in @p lookup
      //!
      //! @return a tuple with the two ranges
      template< concepts::range R1, concepts::range R2>
      [[nodiscard]] auto divide_first( R1&& range, R2&& lookup)
      {
         return divide_first( std::forward< R1>( range), std::forward< R2>( lookup), std::equal_to<>{});
      }

      //! Divide @p range in two parts [range-first, divider), [divider, range-last).
      //! where divider is the first occurrence of the whole match in @p to_find,
      //! hence, second part (if not empty) starts with the content of `to_find` 
      //!
      //! @return a tuple with the two ranges
      template< concepts::range R1, concepts::range R2>
      [[nodiscard]] auto divide_search( R1&& range, R2&& to_find)
      {
         auto divider = search( range, to_find);
         return std::make_tuple( range::make( std::begin( range), std::begin( divider)), divider);
      }

      //! Divide @p range in two parts [range-first, intersection-end), [intersection-end, range-last).
      //! where the first range is the intersection of the @p range and @p lookup
      //! and the second range is the complement of @range with regards to @p lookup
      //!
      //! @return a tuple with the two ranges
      template< concepts::range R1, concepts::range R2, typename F>
      [[nodiscard]] auto intersection( R1&& range, R2&& lookup, F functor)
      {
         auto lambda = [&]( auto&& v){
            return find_if( lookup, [&]( auto&& l){
               return functor( v, l);
            });
         };
         return stable::partition( std::forward< R1>( range), lambda);
      }

      //! Divide @p range in two parts [range-first, intersection-end), [intersection-end, range-last).
      //! where the first range is the intersection of the @p range and @p lookup
      //! and the second range is the complement of @range with regards to @p lookup
      //!
      //! @return a tuple with the two ranges
      template< concepts::range R1, concepts::range R2>
      [[nodiscard]] auto intersection( R1&& range, R2&& lookup)
      {
         return intersection( std::forward< R1>( range), std::forward< R2>( lookup), std::equal_to<>{});
      }


      template< concepts::range R, typename F>
      [[nodiscard]] auto max( R&& range, F functor)
      {
         // Just to make sure range is not an rvalue container. we could use enable_if instead
         auto result = range::make( std::forward< R>( range));

         return range::make( std::max_element( std::begin( result), std::end( result), functor), std::end( result));
      }

      template< concepts::range R>
      [[nodiscard]] auto max( R&& range)
      {
         return max( std::forward< R>( range), std::less<>{});
      }

      template< concepts::range R, typename F>
      [[nodiscard]] auto min( R&& range, F functor)
      {
         // Just to make sure range is not an rvalue container. we could use enable_if instead.
         auto result = range::make( std::forward< R>( range));

         return range::make( std::min_element( std::begin( result), std::end( result), functor), std::end( result));
      }

      template< concepts::range R>
      [[nodiscard]] auto min( R&& range)
      {
         return min( std::forward< R>( range), std::less<>{});
      }


      //! @return true if all elements in @p other is found in @p source
      template< concepts::range R1, concepts::range R2>
      [[nodiscard]] bool includes( R1&& source, R2&& other)
      {
         return all_of( other, [&]( const auto& value)
         { 
            return find( std::forward< R1>( source), value);
         });
      }

      //! Uses @p compare to compare for equality
      //!
      //! @return true if all elements in @p other is found in @p source
      template< concepts::range R1, concepts::range R2, typename Compare>
      [[nodiscard]] bool includes( R1&& source, R2&& other, Compare compare)
      {
         return all_of( other, [&]( const auto& v)
         { 
            return find_if( source, [&]( const auto& s){ return compare( s, v);});
         });

      }

      //! @return true if all elements in the range compare equal
      template< concepts::range R>
      [[nodiscard]] bool uniform( R&& range)
      {
         auto first = std::begin( range);

         for( auto&& value : range)
            if( value != *first)
               return false;

         return true;
      }

      //! @return true if @p range1 includes @p range2, AND @p range2 includes @p range1
      template< concepts::range R1, concepts::range R2, typename Compare>
      [[nodiscard]] bool uniform( R1&& range1, R2&& range2, Compare comp)
      {
         return includes( std::forward< R1>( range1), std::forward< R2>( range2), comp)
               && includes( std::forward< R2>( range2), std::forward< R1>( range1), predicate::inverse( comp));
      }

      template< concepts::range Range, typename T>
      [[nodiscard]] platform::size::type count( Range&& range, T&& value)
      {
         return std::count( std::begin( range), std::end( range), std::forward< T>( value));
      }

      template< concepts::range Range, typename Predicate>
      [[nodiscard]] platform::size::type count_if( Range&& range, Predicate predicate)
      {
         return std::count_if( std::begin( range), std::end( range), predicate);
      }

      namespace numeric
      {
         template< concepts::range R, typename T>
         void iota( R&& range, T value)
         {
            std::iota( std::begin( range), std::end( range), value);
         }
      } // numeric

      namespace lexicographical
      {
         template< concepts::range A, concepts::range B> 
         [[nodiscard]] auto compare( A&& a, B&& b)
         {
            return std::lexicographical_compare(
               std::begin( a), std::end( a),
               std::begin( b), std::end( b));
         }
      } // lexicographical

   } // common::algorithm
} // casual


