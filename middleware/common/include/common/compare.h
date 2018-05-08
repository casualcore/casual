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
            using member_tie = decltype( std::declval< const T&>().tie());

            template< typename T> 
            using has_member_tie = traits::detect::is_detected< member_tie, T>;
            

            template< typename T>
            constexpr auto get_tie( const T& value, std::enable_if_t< has_member_tie< T>::value>* = 0) 
            {
               return value.tie();
            }

            template< typename T> 
            using free_tie = decltype( tie( std::declval< const T&>()));

            template< typename T> 
            using has_free_tie = traits::detect::is_detected< free_tie, T>;

            template< typename T>
            constexpr auto get_tie( const T& value, std::enable_if_t< has_free_tie< T>::value && ! has_member_tie< T>::value>* = 0)  
            {
               return tie( value);
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
