//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include <utility>
#include <functional>

#include "../../../../thirdparty/function2/include/function2/function2.hpp"

namespace casual
{
   namespace common
   {
      template <typename... Signatures>
      using unique_function = fu2::unique_function< Signatures...>;
      
      template <typename... Signatures>
      using function = fu2::function< Signatures...>; 


#if __cplusplus >= 201703L
      using std::apply;
      using std::invoke;
#else 

      namespace detail
      {
         //! memberfunction
         template< typename Base, typename R, typename Derived, typename... Args>
         constexpr decltype( auto) invoke_implementation( R Base::*pmf, Derived& derived, Args&&... args)
         {
            return ( derived.*pmf)( std::forward<Args>(args)...);
         }

         template< typename Base, typename R, typename Derived, typename... Args>
         constexpr decltype( auto) invoke_implementation( R Base::*pmf, Derived* derived, Args&&... args)
         {
            return ( derived->*pmf)( std::forward<Args>(args)...);
         }

         //! free function/functor
         template< typename F, typename... Args>
         constexpr decltype( auto) invoke_implementation( F&& function, Args&&... args)
         {
            return std::forward<F>( function)( std::forward<Args>(args)...);
         }

      } // detail


      template< typename F, typename... Args>
      constexpr decltype( auto) invoke( F&& function, Args&&... args)
      {
         return detail::invoke_implementation( std::forward< F>( function), std::forward< Args>( args)...);
      }

      namespace detail 
      {
         template< typename C, typename Tuple, std::size_t... I>
         constexpr decltype(auto) apply_implementation( C&& callable, Tuple&& t, std::index_sequence< I...>)
         {
            return common::invoke( std::forward< C>( callable), std::get< I>( std::forward< Tuple>( t))...);
         }
      } // detail


      //! @note "inspired" by cppreference.com 
      template< typename C, typename Tuple>
      constexpr decltype( auto) apply( C&& callable, Tuple&& t)
      {
         return detail::apply_implementation(
            std::forward< C>( callable), std::forward<Tuple>( t),
            std::make_index_sequence< std::tuple_size< std::remove_reference_t< Tuple>>::value>{});
      }
#endif

   } // common
} // casual


