//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"

namespace casual
{
   namespace common::algorithm
   {
         namespace detail
      {
         namespace coalesce
         {


            template< typename T>
            auto empty( T&& value, traits::priority::tag< 2>) -> decltype( value.empty()) 
            { return value.empty();}

            template< typename T>
            auto empty( T&& value, traits::priority::tag< 1>) -> decltype( ! value.has_value()) 
            { return ! value.has_value();}

            template< typename T>
            auto empty( T&& value, traits::priority::tag< 0>) -> decltype( value == nullptr)
            { return value == nullptr;}

            template< typename T>
            decltype( auto) implementation( T&& value)
            {
               return std::forward< T>( value);
            }

            template< typename T, typename... Args>
            auto implementation( T&& value, Args&&... args) ->
               std::conditional_t<
                  traits::is::same_v< T, Args...>,
                  T, // only if T and Args are exactly the same, we use T, otherwise we convert to common type
                  std::common_type_t< T, Args...>>
            {
               if( coalesce::empty( value, traits::priority::tag< 2>{}))
                  return implementation( std::forward< Args>( args)...);

               return std::forward< T>( value);
            }

         } // coalesce

      } // detail

      //! Chooses the first argument that is not 'empty'
      //!
      //! @note the return type will be the common type of all types
      //!
      //! @return the first argument that is not 'empty'
      template< typename T, typename... Args>
      decltype( auto) coalesce( T&& value,  Args&&... args)
      {
         return detail::coalesce::implementation( std::forward< T>( value), std::forward< Args>( args)...);
      }

   } // common::algorithm
} // casual