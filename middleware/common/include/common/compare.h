//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/traits.h"

namespace casual
{
   namespace common 
   {
      namespace compare 
      {
         namespace detail
         {
            template< typename T>
            constexpr auto get_tie( const T& value, traits::priority::tag< 0>)
               -> decltype( tie( value))
            {
               return tie( value);
            }

            template< typename T>
            constexpr auto get_tie( const T& value, traits::priority::tag< 1>)
               -> decltype( value.tie())
            {
               return value.tie();
            }

            template< typename T>
            constexpr auto get_tie( const T& value) -> decltype( get_tie( value, traits::priority::tag< 1>{}))
            {
               return get_tie( value, traits::priority::tag< 1>{});
            }

         } // detail

         template< typename T>
         struct Equality
         {
            constexpr friend bool operator == ( const T& lhs, const T& rhs) { return compare::detail::get_tie( lhs) == compare::detail::get_tie( rhs);}
            constexpr friend bool operator != ( const T& lhs, const T& rhs) { return compare::detail::get_tie( lhs) != compare::detail::get_tie( rhs);}
         };

         template< typename T>
         struct Order
         {
            constexpr friend bool operator < ( const T& lhs, const T& rhs)  { return compare::detail::get_tie( lhs) <  compare::detail::get_tie( rhs);}
            constexpr friend bool operator <= ( const T& lhs, const T& rhs) { return compare::detail::get_tie( lhs) <= compare::detail::get_tie( rhs);}
            constexpr friend bool operator > ( const T& lhs, const T& rhs)  { return compare::detail::get_tie( lhs) >  compare::detail::get_tie( rhs);}
            constexpr friend bool operator >= ( const T& lhs, const T& rhs) { return compare::detail::get_tie( lhs) >= compare::detail::get_tie( rhs);}
         };
         
      } // compare 

      template< typename T>
      struct Compare : compare::Equality< T>, compare::Order< T>
      {

      };

   } // common 
} // casual
