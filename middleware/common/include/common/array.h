//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <array>
#include <type_traits>

namespace casual
{
   namespace common::array
   {

      //! return an array of correct type (common type) and size.
      //! TODO maintainability - remove/replace when std::make_array arrives.
      template< typename... Ts> 
      constexpr auto make( Ts&&... ts)
      {
         using array_type = std::array< std::common_type_t< Ts...>, sizeof...(Ts)>;
         return array_type{ std::forward< Ts>( ts)...};
      }

      namespace force
      {
         template< typename T, typename... Ts> 
         constexpr auto make( Ts&&... ts)
         {
            using array_type = std::array< T, sizeof...(Ts)>;
            return array_type{ std::forward< Ts>( ts)...};
         }
      } // force

   } // common::array
} // casual