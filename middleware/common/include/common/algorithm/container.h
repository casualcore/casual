//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"

namespace casual
{
   namespace common::algorithm::container
   {
      namespace move
      {
         namespace detail
         {
            template< typename C> 
            C& back( C& container) { return container;}

            template< typename C, typename T, typename... Ts> 
            C& back( C& container, T&& t, Ts&&... ts)
            {
               container.push_back( std::move( t)); 
               return back( container, std::forward< Ts>( ts)...);
            }
         } // detail
         
         template< typename C, typename... Ts> 
         C& back( C& container, Ts&&... ts)
         {
            return detail::back( container, std::forward< Ts>( ts)...);
         }
      } // move

      namespace emplace
      {
         template< typename C, typename... Ts>
         decltype( auto) back( C&& container, Ts&&... ts)
         {
            ( container.emplace_back( std::forward< Ts>( ts)), ...);
            return std::forward< C>( container);
         }

         template< typename C, typename... Ts>
         C initialize( Ts&&... ts)
         {
            return emplace::back( C{}, std::forward< Ts>( ts)...);
         }
      } // emplace

      namespace detail
      {
         // take care of iterator
         template< typename C, typename Iter>
         auto extract( C& container, Iter where, traits::priority::tag< 1>)
            -> traits::remove_cvref_t< decltype( *container.erase( where))>
         {
            auto result = std::move( *where);
            container.erase( where);
            return result;
         }

         // take care of a range
         template< typename C, typename R>
         auto extract( C& container, R range, traits::priority::tag< 0>)
            -> decltype( void( container.erase( std::begin( range), std::end( range))), C{})
         {
            C result;

            if constexpr( traits::has::reserve_v< decltype( result)>)
               result.reserve( range.size());
            
            std::move( std::begin( range), std::end( range), std::back_inserter( result));
            container.erase( std::begin( range), std::end( range));
            return result;
         }
      } // detail

      //! extracts `where` from the `container`.
      //! @returns 
      //!   * a value if `where` is an iterator
      //!   * a vector with the extracted values if `where` is a range.
      template< typename C, typename W>
      [[nodiscard]] auto extract( C& container, W&& where)
         -> decltype( detail::extract( container, std::forward< W>( where), traits::priority::tag< 1>{}))
      {
         return detail::extract( container, std::forward< W>( where), traits::priority::tag< 1>{});
      }

      //! Trims @p container so it matches @p range
      //!
      //! @return range that matches the trimmed @p container
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

      //! erases every element in the `container` that are 'equal' to `value`,
      //! which could be none.
      //! @return `container`
      template< typename C, typename T>
      C& erase( C& container, const T& value)
      {
         while( auto found = algorithm::find( container, value))
            container.erase( std::begin( found));
         return container;
      }

      //! Erases occurrences from an associative container that
      //! fulfill the predicate
      //!
      //! @param container a container
      //! @param predicate that takes C::value_type as parameter and returns bool
      //! @return the container
      template< typename C, typename P>
      C& erase_if( C& container, P&& predicate)
      {
         for( auto current = std::begin( container); current != std::end( container);)
         {
            if( predicate( *current))
               current = container.erase( current);
            else
               ++current;
         }
         return container;
      }

   } // common::algorithm::container
} // casual