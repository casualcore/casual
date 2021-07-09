//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

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
         namespace detail
         {
            template< typename C> 
            void back( C& container) {}

            template< typename C, typename T, typename... Ts> 
            void back( C& container, T&& t, Ts&&... ts)
            {
               container.emplace_back( std::forward< T>( t)); 
               back( container, std::forward< Ts>( ts)...);
            }
         } // detail

         template< typename C, typename... Ts>
         C initialize( Ts&&... ts)
         {
            C result;
            detail::back( result, std::forward< Ts>( ts)...);
            return result;
         }
      } // emplace
   } // common::algorithm::container
} // casual