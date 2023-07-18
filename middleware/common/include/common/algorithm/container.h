//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"
#include "common/concepts.h"
#include "common/algorithm.h"

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
               container.push_back( std::forward< T>( t)); 
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

      template< typename C, typename Iter>
      C& erase( C& container, Iter where) requires std::same_as< traits::iterator_t< C>, Iter>
      {
         container.erase( where);
         return container;
      }

      //! erases every element in the `container` that are 'equal' to `value`,
      //! which could be none.
      //! @return `container`
      template< typename C, typename T>
      auto erase( C& container, const T& value) -> decltype( void( range::front( container) == value), container)
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

      //! appends `range` to `output`.
      //! @return output
      template< concepts::range R, concepts::container::insert Out>
      decltype( auto) append( R&& range, Out&& output)
      {
         output.insert( std::end( output), std::begin( range), std::end( range)); 
         return std::forward< Out>( output);
      }

      namespace move
      {
         //! appends `range` to `output`, by moving all values
         //! @return output
         template< concepts::range R, concepts::range Out>
         decltype( auto) append( R range, Out&& output)
         {
            auto movable_range = range::move( range);
            output.reserve( output.size() + movable_range.size());
            algorithm::copy( movable_range, std::back_inserter( output));
            return std::forward< Out>( output);
         }

      } // move



      //! @returns a container of type `Container` with the copied content of `range`
      template< typename Container, typename Range>
      auto create( Range&& range)
      {
         return Container( std::begin( range), std::end( range));
      }

      namespace vector
      {
         //! @returns a `std::vector< _range::value_type_>` with the copied content of `range`
         template< typename Range>
         auto create( Range&& range)
         {
            using result_typ = std::vector< traits::iterable::value_t< Range>>;
            return container::create< result_typ>( std::forward< Range>( range));
         }

         namespace reference
         {
            template< typename Range>
            auto create( Range&& range)
            {
               using result_typ = std::vector< std::reference_wrapper< std::remove_reference_t< decltype( *std::begin( range))>>>;
               return container::create< result_typ>( std::forward< Range>( range));
            }
         } // reference
         
      } // vector

      namespace detail
      {  
         template< typename T, std::same_as< T> U>
         void compose( std::vector< T>& result, U value)
         {
            result.push_back( std::move( value));
         }

         template< typename T, common::concepts::range R>
         void compose( std::vector< T>& result, R value)
         {
            container::move::append( std::move( value), result);
         }

         template< typename T>
         auto compose_init( std::vector< T> value) { return value;}
            
         template< typename T>
         auto compose_init( T value) //-> decltype( std::vector{ std::move( value)})
         {
            std::vector< T> result;
            result.push_back( std::move( value));
            return result;
         }
      } // detail
      

      //! 'composes' multiple (rvalue) containers into one.
      //! TODO how to construct a suitable concept/requires for this? 
      template< typename T, typename... Ts> 
      auto compose( T t, Ts... ts) -> decltype( detail::compose_init( std::move( t)))
      {
         auto result = detail::compose_init( std::move( t));
         ( ... ,  detail::compose( result, std::move( ts)) );

         return result;
      }


     namespace sort
     {
         //! a convenience function to make a container sorted and unique
         template< typename C>
         C& unique( C& container)
         {
            return container::trim( container, algorithm::unique( algorithm::sort( container)));    
         }
     } // sort

   } // common::algorithm::container
} // casual