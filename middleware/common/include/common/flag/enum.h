//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include <type_traits>
#include <concepts>
#include <string_view>
#include <utility>

namespace casual
{
   namespace common::flag
   {
      //! We're using strongly typed enums as flag abstraction. This makes it harder to
      //! produce bugs.
      //! 
      //! To opt in to use an enum as flag declare:
      //!
      //! `consteval casual_enum_as_flag( <your enum type>);
      //!
      //! You can also opt in and use another flag as superset:
      //!
      //! `consteval <the superset type> casual_enum_as_flag_superset( <your enum type>);
      //!
      //! This will use the superset enum `description` function when printing your enum type.
      //! You have to ensure that your enum type is a strict subset of the superset enum.

      namespace detail
      {
         template< typename T>
         concept enum_as_flag = std::is_enum_v< T> && requires( T e)
         {
            casual_enum_as_flag( e);
         };

         template< typename T>
         concept has_description = enum_as_flag< T> && requires( T e)
         {
            { description( e)} -> std::same_as< std::string_view>;
         }; 

         template< typename T>
         concept enum_as_flag_superset = std::is_enum_v< T> && requires( T e)
         {
            { casual_enum_as_flag_superset( e)} -> has_description;
         };

         template< typename T>
         concept can_use_description = has_description< T> || enum_as_flag_superset< T>; 


         template< can_use_description T>
         constexpr auto description_type( T flag)
         {
            if constexpr( enum_as_flag_superset< T>)
               return static_cast< decltype( casual_enum_as_flag_superset( T{}))>( flag);
            else
               return flag;
         }

      } // detail

      template< typename T>
      concept enum_flag_like = detail::enum_as_flag< T> || detail::enum_as_flag_superset< T>;
      
   } // common::flag
   
} // casual

template< casual::common::flag::enum_flag_like T>
constexpr auto operator | ( T lhs, T rhs)
{
   return static_cast< T>( std::to_underlying( lhs) | std::to_underlying( rhs));
}

template< casual::common::flag::enum_flag_like T>
constexpr auto& operator |= ( T& lhs, T rhs)
{
   return lhs = static_cast< T>( std::to_underlying( lhs) | std::to_underlying( rhs));
}

template< casual::common::flag::enum_flag_like T>
constexpr auto operator & ( T lhs, T rhs)
{
   return static_cast< T>( std::to_underlying( lhs) & std::to_underlying( rhs));
}

template< casual::common::flag::enum_flag_like T>
constexpr auto& operator -= ( T& lhs, T rhs)
{
   return lhs = static_cast< T>( std::to_underlying( lhs) & ~std::to_underlying( rhs));
}


