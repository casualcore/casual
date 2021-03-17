//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

namespace casual
{
   namespace common::algorithm::compare
   {
      namespace detail
      {
         template< typename V, typename T>
         constexpr bool any( V&& value, T&& t)
         {
            return value == t;
         }

         template< typename V, typename T, typename... Ts>
         constexpr bool any( V&& value, T&& t, Ts&&... ts)
         {
            return value == t || any( std::forward< V>( value), std::forward< Ts>( ts)...);
         }
         
      } // detail
      
      //! @returns true if `value` is equal to ane other `values`
      template< typename V, typename... Vs>
      constexpr bool any( V&& value, Vs&&... values)
      {
         return detail::any( std::forward< V>( value), std::forward< Vs>( values)...);
      }         

   } // common::algorithm::compare
} // casual