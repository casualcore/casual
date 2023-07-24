//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"

#include <type_traits>


#include <vector>
#include <deque>
#include <list>

#include <set>
#include <map>

#include <stack>
#include <queue>

#include <unordered_map>
#include <unordered_set>

namespace casual
{
   namespace common::traits
   {
      namespace priority
      {
         template< size_t I> 
         struct tag : tag<I-1> {};
         
         template<> 
         struct tag< 0> {};
      } // priority

      namespace detail
      {
         template< typename R, typename ...Args>
         struct function
         {
            //! @returns number of arguments
            constexpr static auto arguments() -> decltype( sizeof...(Args))
            {
               return sizeof...(Args);
            }

            using decayed = std::tuple< std::decay_t< Args>...>;

            constexpr static auto tuple() { return std::tuple< std::decay_t< Args>...>{}; }

            using result_type = R;

            template< platform::size::type index>
            struct argument
            {
               using type = std::tuple_element_t< index, std::tuple< Args...>>;
            };

            template< platform::size::type index>
            using argument_t = typename argument< index>::type;
         };
      }

      namespace has
      {
         template< typename T>
         concept call_operator = requires
         {
            &T::operator();
         };
      }

      template<typename T>
      struct function {};

      template< has::call_operator T>
      struct function< T> : function< decltype( &T::operator())>
      {
      };

      template< typename T>
      struct function< std::reference_wrapper< T>> : function< T>
      {
      };

      //! const functor specialization
      template< typename C, typename R, typename ...Args>
      struct function< R(C::*)(Args...) const> : detail::function< R, Args...>
      {
      };

      //! non const functor specialization
      template< typename C, typename R, typename ...Args>
      struct function< R(C::*)(Args...)> : detail::function< R, Args...>
      {
      };

      //! free function specialization
      template< typename R, typename ...Args>
      struct function< R(*)(Args...)> : detail::function< R, Args...>
      {
      };

      template< typename T> 
      struct type 
      {
         constexpr static bool nothrow_default_constructible = std::is_nothrow_default_constructible_v< T>;
         constexpr static bool nothrow_construct = std::is_nothrow_constructible_v< T>;
         constexpr static bool nothrow_move_assign = std::is_nothrow_move_assignable_v< T>;
         constexpr static bool nothrow_move_construct = std::is_nothrow_move_constructible_v< T>;
         constexpr static bool nothrow_copy_assign = std::is_nothrow_copy_assignable_v< T>;
         constexpr static bool nothrow_copy_construct = std::is_nothrow_copy_constructible_v< T>;
      };


      template< typename T>
      struct is_by_value_friendly : std::bool_constant< 
         std::is_trivially_copyable_v< std::remove_cvref_t< T>>
         && sizeof( std::remove_cvref_t< T>) <= platform::size::by::value::max> {};


      template< typename T>
      struct by_value_or_const_ref : std::conditional< is_by_value_friendly< T>::value, std::remove_cvref_t< T>, const T&> {};

      template< typename T>
      using by_value_or_const_ref_t = typename by_value_or_const_ref< T>::type;



      //! usable in else branch in constexpr if context
      template< typename T> 
      struct dependent_false : std::false_type {};


      struct unmovable
      {
         unmovable() = default;
         unmovable( unmovable&&) = delete;
         unmovable& operator = ( unmovable&&) = delete;
      };

      struct uncopyable
      {
         uncopyable() = default;
         uncopyable( const uncopyable&) = delete;
         uncopyable& operator = ( const uncopyable&) = delete;
      };

      struct unrelocatable : unmovable, uncopyable {};

      namespace is
      {
         template< typename T>
         concept function = requires 
         {
            { traits::function< T>::arguments()} -> std::convertible_to< platform::size::type>;
         };
      } // is

      //! should be called `common::type` but the name clashes on 
      //! 'common' is to severe.
      namespace convertible
      {
         template< typename... Ts>
         constexpr auto type( Ts&&... ts) -> std::common_type_t< Ts...>;
      } // convertible


   } // common::traits
} // casual


