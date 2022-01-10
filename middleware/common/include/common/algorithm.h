//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/range.h"
#include "common/traits.h"
#include "casual/platform.h"
#include "common/cast.h"
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

      template< typename R, typename = std::enable_if_t< common::traits::is::iterable_v< R>>>
      decltype( auto) reverse( R&& range)
      {
         std::reverse( std::begin( range), std::end( range));
         return std::forward< R>( range);
      }

      namespace detail
      {
         template< typename P, std::enable_if_t< traits::is::iterable_v< P>>* dummy = nullptr>
         auto pivot( P&& pivot) { return std::begin( pivot);}

         template< typename P, std::enable_if_t< traits::is::iterator_v< P>>* dummy = nullptr>
         auto pivot( P&& pivot) { return pivot;}

      } // detail

      //! Rotates the range based on the pivot.
      //! @return a tuple of { [begin( range), pivot), pivot, end( range)]}
      template< typename R, typename P>
      auto rotate( R&& range, P&& pivot)
      {
         auto middle = std::rotate( std::begin( range), detail::pivot( pivot), std::end( range));
         return std::make_tuple( range::make( std::begin( range), middle), range::make( middle, std::end( range)));
      }

      template< typename R, typename C>
      auto sort( R&& range, C compare) 
         -> decltype( void( std::sort( std::begin( range), std::end( range), compare)), std::forward< R>( range))
      {
         std::sort( std::begin( range), std::end( range), compare);
         return std::forward< R>( range);
      }

      template< typename R>
      auto sort( R&& range) 
         -> decltype( void( std::sort( std::begin( range), std::end( range))), std::forward< R>( range))
      {
         std::sort( std::begin( range), std::end( range));
         return std::forward< R>( range);
      }

      template< typename R, typename C>
      auto stable_sort( R&& range, C compare)
         -> decltype( void( std::stable_sort( std::begin( range), std::end( range), compare)), std::forward< R>( range))
      {
         std::stable_sort( std::begin( range), std::end( range), compare);
         return std::forward< R>( range);
      }

      template< typename R>
      auto stable_sort( R&& range) 
         -> decltype( void( std::stable_sort( std::begin( range), std::end( range))), std::forward< R>( range))
      {
         std::stable_sort( std::begin( range), std::end( range));
         return std::forward< R>( range);
      }

      //! fills the container with `value`
      //! @returns the range
      template< typename R, typename V>
      decltype( auto) fill( R&& range, V&& value)
      {
         std::fill( std::begin( range), std::end( range), std::forward< V>( value));
         return std::forward< R>( range);
      }

      namespace detail
      {
         template< typename T>
         constexpr bool has_resize_copy_v = traits::has::resize_v< T> &&
            std::is_default_constructible_v< traits::remove_cvref_t< traits::iterable::value_t< T>>>;

         namespace output
         {
            template< typename C> 
            auto iterator( C& value, traits::priority::tag< 2>)
               -> std::enable_if_t< traits::has::push_back_v< C>, decltype( std::back_inserter( value))>
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
      template< typename R, typename Out> 
      auto copy( R&& range, Out&& output) -> decltype( detail::copy( std::forward< R>( range), std::forward< Out>( output), traits::priority::tag< 2>{}))
      {
         return detail::copy( std::forward< R>( range), std::forward< Out>( output), traits::priority::tag< 2>{});
      }



      template< typename R, typename Out, typename P> 
      auto copy_if( R&& range, Out&& output, P predicate)
      {
         return std::copy_if( std::begin( range), std::end( range), detail::output::iterator( std::forward< Out>( output)), predicate);
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

      //! Copies from @p source to @p destination
      //!   size of destination dictates the maximum that will be
      //!   copied
      //!
      //! @param source
      //! @param destination sets the maximum what will be copied
      template< typename Range1, typename Range2>
      void copy_max( Range1&& source, Range2&& destination)
      {
         copy_max( source, range::size( destination), std::begin( destination));
      }

      //! move range to out.
      //! if out has push_back it will be used, hance append.
      template< typename R, typename Out>
      decltype( auto) move( R&& range, Out&& out)
      {
         std::move( std::begin( range), std::end( range), detail::output::iterator( out));
         return std::forward< Out>( out);
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
         auto transform( R&& range, C& container, T transform, range::category::container)
         {
            container.reserve( range::size( range) + container.size());
            std::transform( std::begin( range), std::end( range), std::back_inserter( container), transform);
            return range::make( std::end( container) - range::size( range), std::end( container));
         }

         template< typename R, typename C, typename T>
         auto transform( R&& range, C&& container, T transform, range::category::associative)
         {
            container.reserve( range::size( range) + container.size());
            for( auto& value: range)
               container.emplace( transform( value));

            return std::forward< C>( container);
         }

         template< typename R, typename O, typename T>
         decltype( auto) transform( R&& range, O&& output, T transform, range::category::fixed)
         {
            assert( range::size( output) >= range::size( range));
            std::transform( std::begin( range), std::end( range), std::begin( output), transform);
            return std::forward< O>( output);
         }

         template< typename R, typename O, typename T>
         auto transform( R&& range, O&& output, T transform, range::category::output_iterator)
         {
            std::transform( std::begin( range), std::end( range), output, transform);
            return std::forward< O>( output);
         }
      } // detail

      //! Transform @p range to @p container, using @p transform
      //!
      //! @param range source range/container
      //! @param container output/holder container
      //!
      //! @return range containing the inserted transformed values, previous values in @p container is excluded.
      template< typename R, typename O, typename T>
      decltype( auto) transform( R&& range, O&& output, T&& transform)
      {
         return detail::transform( std::forward< R>( range), std::forward< O>( output), std::forward< T>( transform), range::category::tag_t< O>{} );
      }

      //! Transform @p range, using @p transform
      //!
      //! @param range source range/container
      //!
      //! @return std::vector with the transformed values
      template< typename R, typename T>
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
      template< typename R, typename Out, typename T, typename P>
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

      template< typename R, typename T, typename P>
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
      template< typename R>
      [[nodiscard]] auto unique( R&& range)
      {
         return range::make( std::begin( range), std::unique( std::begin( range), std::end( range)));
      }

      template< typename R, typename P>
      [[nodiscard]] auto unique( R&& range, P predicate)
      {
         return range::make( std::begin( range), std::unique( std::begin( range), std::end( range), predicate));
      }
      //! @}


      namespace detail
      {
         template< typename R, typename T>
         using iterator_value_compare = decltype( *std::begin( std::declval< R&>()) == std::declval< T&>());

         template< typename R1, typename R2>
         using iterator_iterator_swap = decltype( std::swap( *std::begin( std::declval< R1&>()), *std::begin( std::declval< R2&>())));
      }

      template< typename R, typename T, std::enable_if_t< common::traits::detect::is_detected< detail::iterator_value_compare, R, T>::value>* dummy = nullptr>
      [[nodiscard]] auto remove( R&& range, const T& value)
      {
         return range::make( std::begin( range), std::remove( std::begin( range), std::end( range), value));
      }

      //! Removes the unwanted range from the source
      template< typename R1, typename R2, std::enable_if_t< common::traits::detect::is_detected< detail::iterator_iterator_swap, R1, R2>::value>* dummy = nullptr>
      [[nodiscard]] auto remove( R1&& source, R2&& unwanted)
      {
         auto last = std::rotate( std::begin( unwanted), std::end( unwanted), std::end( source));
         
         return range::make( std::begin( source), last);
      }

      template< typename R, typename P>
      [[nodiscard]] auto remove_if( R&& range, P predicate)
      {
         return range::make( std::begin( range), std::remove_if( std::begin( range), std::end( range), predicate));
      }


      template< typename R1, typename R2, typename P>
      [[nodiscard]] bool equal( R1&& lhs, R2&& rhs, P predicate)
      {
         return std::equal( std::begin( lhs), std::end( lhs), 
            std::begin( rhs), std::end( rhs), predicate);
      }


      template< typename R1, typename R2>
      [[nodiscard]] bool equal( R1&& lhs, R2&& rhs)
      {
         return std::equal( std::begin( lhs), std::end( lhs), 
            std::begin( rhs), std::end( rhs));
      }


      template< typename R, typename T>
      [[nodiscard]] constexpr T accumulate( R&& range, T result)
      {
         // TODO maintainence: use std::accumulate when c++20
         for( auto& value : range)
            result = std::move( result) + value;
         return result;
      }

      template< typename R, typename T, typename F>
      [[nodiscard]] constexpr T accumulate( R&& range, T result, F&& functor)
      {
         // TODO maintainence: use std::accumulate when c++20
         for( auto& value : range)
            result = functor( std::move( result), value);
         return result;
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

      //! applies `functor` to all elements
      template< typename R, typename F>
      decltype( auto) for_each( R&& range, F&& functor)
      {
         std::for_each( std::begin( range), std::end( range), std::forward< F>( functor));
         return std::forward< R>( range);
      }

      //! applies `functor` to all elements that `predicate` is true.
      template< typename R, typename F, typename P>
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
      template< typename R, typename F>
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
      template< typename R, typename F>
      decltype( auto) for_each_while( R&& range, F functor)
      {  
         for( auto current = range::make( range); current && functor( *current); ++current)
            ; // no-op
         
         return std::forward< R>( range);
      }

      //! applies `functor` on all elements that are equal to `value`
      template< typename R, typename V, typename F>
      decltype( auto) for_each_equal( R&& range, V&& value, F functor)
      {
         for( auto&& element : range)
         {
            if( element == value)
               functor( element);
         }
         return std::forward< R>( range);
      }

      template< typename R, typename F>
      auto for_each_n( R&& range, platform::size::type n, F functor) -> decltype( range::make( std::forward< R>( range)))
      {
         if( range::size( range) <= n)
            return for_each( range::make( range), functor);
         else
            return for_each( range::make( std::begin( range), n), functor);
      }

      //! applies `functor` on all occurencies, and call `interleave` between each
      template< typename R, typename F, typename I>
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
         auto find( R&& range, const T& value, traits::priority::tag< 1>) 
            -> decltype( range::make( range.find( value), std::end( range)))
         {
            return range::make( range.find( value), std::end( range));
         }

         //! non associate container specialization
         template< typename R, typename T>
         auto find( R&& range, const T& value, traits::priority::tag< 0>)
            -> decltype( range::make( std::find( std::begin( range), std::end( range), value), std::end( range)))
         {
            return range::make( std::find( std::begin( range), std::end( range), value), std::end( range));
         }
      } // detail

      //! @returns a range [found, end( range)) - empty range if not found.
      template< typename R, typename T>
      auto find( R&& range, const T& value)
         -> decltype( detail::find( std::forward< R>( range), value, traits::priority::tag< 1>{}))
      {
         return detail::find( std::forward< R>( range), value, traits::priority::tag< 1>{});
      }

      template< typename R, typename P>
      auto find_if( R&& range, P predicate)
      {
         return range::make( std::find_if( std::begin( range), std::end( range), predicate), std::end( range));
      }


      template< typename R, typename P>
      auto adjacent_find( R&& range, P predicate)
      {
         return range::make( std::adjacent_find( std::begin( range), std::end( range), predicate), std::end( range));
      }

      template< typename R>
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

      //! @returns true if `value` is found in `range` - false othervise
      template< typename R, typename T>
      auto contains( R&& range, const T& value)
         -> decltype( detail::contains( std::forward< R>( range), value, traits::priority::tag< 2>{}))
      {
         return detail::contains( std::forward< R>( range), value, traits::priority::tag< 2>{});
      }


      template< typename R, typename V>
      auto replace( R&& range, V&& old_value, V&& new_value)
      {
         std::replace( std::begin( range), std::end( range), old_value, new_value);
         return std::forward< R>( range);
      }

      //! appends `range` to `output`.
      //! @return output
      template< typename R, typename Out>
      decltype( auto) append( R&& range, Out&& output)
      {
         static_assert( common::traits::is::iterable_v< R>);

         if constexpr( detail::has_resize_copy_v< Out>)
         {
            auto size = std::distance( std::begin( range), std::end( range));
            output.resize( output.size() + size);
            std::copy( std::begin( range), std::end( range), std::end( output) - size);
         }
         else if constexpr( traits::has::push_back_v< Out>)
            std::copy( std::begin( range), std::end( range), std::back_inserter( output));
         else
            static_assert( traits::dependent_false< Out>::value, "output needs to be resizable or have push_back");

         return std::forward< Out>( output);
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

      //! push_back `value` to `output` if it's not allready present
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


      template< typename R, typename O, typename P> 
      auto append_unique( R& range, O&& output, P predicate)
         -> decltype( void( append_unique_value( range::front( range), output, predicate)), std::forward< O>( output))
      {
         for( auto& value : range)
            append_unique_value( value, output, predicate);

         return std::forward< O>( output);
      }

      template< typename R, typename O> 
      auto append_unique( R&& range, O&& output) 
      -> decltype( void( append_unique_value( range::front( range), output)), std::forward< O>( output))
      {
         for( auto& value : range)
            append_unique_value( value, output);

         return std::forward< O>( output);
      }
      
      template< typename R, typename O, typename P> 
      auto append_replace( R&& range, O&& output, P predicate)
         -> decltype( void( append_replace_value( range::front( range), output, predicate)), std::forward< O>( output))
      {
         for( auto& value : range)
            append_replace_value( value, output, predicate);

         return std::forward< O>( output);
      }

      template< typename R, typename O> 
      auto append_replace( R&& range, O&& output) 
         -> decltype( void( append_replace_value( range::front( range), output)), std::forward< O>( output))
      {
         for( auto& value : range)
            append_replace_value( value, output);

         return std::forward< O>( output);
      }

      //! partition `range` based on `predicate`
      //! @returns tuple with [begin( range), partition-end) [partition-end, end( range)] 
      template< typename R, typename P>
      auto partition( R&& range, P predicate)
         -> decltype( std::make_tuple( range::make( range), range::make( std::partition( std::begin( range), std::end( range), predicate), std::end( range))))
      {
         auto middle = std::partition( std::begin( range), std::end( range), predicate);
         return std::make_tuple( range::make( std::begin( range), middle), range::make( middle, std::end( range)));
      }

      //! @returns std::get< 0>( algorithm::partition( range, predicate));
      template< typename R, typename P>
      auto filter( R&& range, P predicate) -> traits::remove_cvref_t< decltype( std::get< 0>( partition( range, predicate)))>
      {
         return std::get< 0>( partition( range, predicate));
      }

      namespace stable
      {
         template< typename R, typename P, typename = std::enable_if_t< common::traits::is::iterable_v< R>>>
         auto partition( R&& range, P predicate)
         {
            auto middle = std::stable_partition( std::begin( range), std::end( range), predicate);
            return std::make_tuple( range::make( std::begin( range), middle), range::make( middle, std::end( range)));
         }

         //! @returns std::get< 0>( algorithm::statple::partition( range, predicate));
         template< typename R, typename P>
         auto filter( R&& range, P predicate) -> traits::remove_cvref_t< decltype( std::get< 0>( partition( range, predicate)))>
         {
            return std::get< 0>( partition( range, predicate));
         }            
      } // stable

      //! Divide @p range in two parts [range-first, divider), [divider, range-last).
      //! where divider is the first occurrence that is equal to @p value
      //!
      //! @return a tuple with the two ranges
      template< typename R, typename T>
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
      template< typename R, typename T>
      [[nodiscard]] auto split( R&& range, const T& value)
      {
         auto result = divide( std::forward< R>( range), value);
         if( ! std::get< 1>( result).empty())
         {
            ++std::get< 1>( result);
         }
         return result;
      }

      //! Divide @p range in two parts [range-first, divider), [divider, range-last).
      //! where divider is the first occurrence where @p predicate is true
      //!
      //! @return a tuple with the two ranges
      template< typename R1, typename P>
      [[nodiscard]] auto divide_if( R1&& range, P predicate)
      {
         auto divider = std::find_if( std::begin( range), std::end( range), predicate);

         return std::make_tuple( range::make( std::begin( range), divider), range::make( divider, std::end( range)));
      }
      

      template< typename R1, typename R2>
      [[nodiscard]] auto search( R1&& range, R2&& to_find)
      {
         auto first = std::search( std::begin( range), std::end( range), std::begin( to_find), std::end( to_find));
         return range::make( first, std::end( range));
      }


      template< typename R1, typename R2, typename F>
      [[nodiscard]] auto find_first_of( R1&& source, R2&& lookup, F functor)
      {
         auto found = std::find_first_of(
               std::begin( source), std::end( source),
               std::begin( lookup), std::end( lookup),
               functor);

         return range::make( found, std::end( source));
      }

      template< typename R1, typename R2>
      [[nodiscard]] auto find_first_of( R1&& target, R2&& source)
      {
         return find_first_of( std::forward< R1>( target), std::forward< R2>( source), std::equal_to<>{});
      }

      //! Divide @p range in two parts [range-first, divider), [divider, range-last).
      //! where divider is the first occurrence found in @p lookup
      //!
      //! @return a tuple with the two ranges
      template< typename R1, typename R2, typename F>
      [[nodiscard]] auto divide_first( R1&& range, R2&& lookup, F functor)
      {
         auto divider =  find_first_of( range, lookup, functor);

         return std::make_tuple( range::make( std::begin( range), std::begin( divider)), divider);
      }

      //! Divide @p range in two parts [range-first, divider), [divider, range-last).
      //! where divider is the first occurrence found in @p lookup
      //!
      //! @return a tuple with the two ranges
      template< typename R1, typename R2>
      [[nodiscard]] auto divide_first( R1&& range, R2&& lookup)
      {
         return divide_first( std::forward< R1>( range), std::forward< R2>( lookup), std::equal_to<>{});
      }

      //! Divide @p range in two parts [range-first, divider), [divider, range-last).
      //! where divider is the first occurrence of the whole match in @p to_find,
      //! hence, second part (if not empty) starts with the content of `to_find` 
      //!
      //! @return a tuple with the two ranges
      template< typename R1, typename R2>
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
      template< typename R1, typename R2, typename F>
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
      template< typename R1, typename R2>
      [[nodiscard]] auto intersection( R1&& range, R2&& lookup)
      {
         return intersection( std::forward< R1>( range), std::forward< R2>( lookup), std::equal_to<>{});
      }


      template< typename R, typename F>
      [[nodiscard]] auto max( R&& range, F functor)
      {
         // Just to make sure range is not an rvalue container. we could use enable_if instead
         auto result = range::make( std::forward< R>( range));

         return range::make( std::max_element( std::begin( result), std::end( result), functor), std::end( result));
      }

      template< typename R>
      [[nodiscard]] auto max( R&& range)
      {
         return max( std::forward< R>( range), std::less<>{});
      }

      template< typename R, typename F>
      [[nodiscard]] auto min( R&& range, F functor)
      {
         // Just to make sure range is not an rvalue container. we could use enable_if instead.
         auto result = range::make( std::forward< R>( range));

         return range::make( std::min_element( std::begin( result), std::end( result), functor), std::end( result));
      }

      template< typename R>
      [[nodiscard]] auto min( R&& range)
      {
         return min( std::forward< R>( range), std::less<>{});
      }


      //! @return true if all elements in @p other is found in @p source
      template< typename R1, typename R2>
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
      template< typename R1, typename R2, typename Compare>
      [[nodiscard]] bool includes( R1&& source, R2&& other, Compare compare)
      {
         return all_of( other, [&]( const auto& v)
         { 
            return find_if( source, [&]( const auto& s){ return compare( s, v);});
         });

      }

      //! @return true if all elements in the range compare equal
      template< typename R>
      [[nodiscard]] bool uniform( R&& range)
      {
         auto first = std::begin( range);

         for( auto&& value : range)
            if( value != *first)
               return false;

         return true;
      }

      //! @return true if @p range1 includes @p range2, AND @p range2 includes @p range1
      template< typename R1, typename R2, typename Compare>
      [[nodiscard]] bool uniform( R1&& range1, R2&& range2, Compare comp)
      {
         return includes( std::forward< R1>( range1), std::forward< R2>( range2), comp)
               && includes( std::forward< R2>( range2), std::forward< R1>( range1), predicate::inverse( comp));
      }

      template< typename Range, typename T>
      [[nodiscard]] platform::size::type count( Range&& range, T&& value)
      {
         return std::count( std::begin( range), std::end( range), std::forward< T>( value));
      }

      template< typename Range, typename Predicate>
      [[nodiscard]] platform::size::type count_if( Range&& range, Predicate predicate)
      {
         return std::count_if( std::begin( range), std::end( range), predicate);
      }

      namespace numeric
      {
         template< typename R, typename T>
         void iota( R&& range, T value)
         {
            std::iota( std::begin( range), std::end( range), value);
         }
      } // numeric

      namespace lexicographical
      {
         template< typename A, typename B> 
         [[nodiscard]] auto compare( A&& a, B&& b)
         {
            return std::lexicographical_compare(
               std::begin( a), std::end( a),
               std::begin( b), std::end( b));
         }
      } // lexicographical

   } // common::algorithm
} // casual


