//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"

#include <system_error>


namespace casual
{
   namespace common
   {
      namespace code
      {
         namespace is
         {
            namespace detail
            {
               template< typename Error, typename Enum>
               auto category( const Error& error, Enum code, traits::priority::tag< 1>) noexcept
                  -> decltype( error.category() == std::error_condition{ code}.category())
               {
                  return error.category() == std::error_condition{ code}.category();
               }
               
               template< typename Error, typename Enum>
               auto category( const Error& error, Enum code, traits::priority::tag< 0>) noexcept
                  -> decltype( error.category() == std::error_code{ code}.category())
               {
                  return error.category() == std::error_code{ code}.category();
               }
            } // detail
            template< typename Enum>
            bool category( const std::error_condition& error) noexcept
            {
               return detail::category( error, static_cast< Enum>( 0), traits::priority::tag< 1>{});
            }

            template< typename Enum>
            bool category( const std::error_code& error) noexcept
            {
               return detail::category( error, static_cast< Enum>( 0), traits::priority::tag< 1>{});
            }
            
         } // is
         

 
      } // code
   } // common
} // casual