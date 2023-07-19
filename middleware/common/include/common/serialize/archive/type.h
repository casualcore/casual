//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <string_view>

namespace casual
{
   namespace common::serialize::archive
   {
      enum class Type : short
      {
         static_need_named,
         static_order_type,
         dynamic_type,
      };

      std::string_view description( Type value) noexcept;

      namespace dynamic
      {
         enum class Type : short
         {
            named,
            order_type,
         };

         std::string_view description( Type value) noexcept;
      } // dynamic


      namespace network
      {
         namespace detail
         {
            template< typename A>
            concept normalizing = requires 
            {  
               typename A::is_network_normalizing;
            };
         } // detail

         //! default instance -> all types that has a typedef of `is_network_normalizing`
         template< typename A>
         struct normalizing : std::bool_constant< detail::normalizing< A>>{};

         template< typename A>
         inline constexpr bool normalizing_v = normalizing< A>::value;
      } // network

   } // common::serialize::archive
} // casual