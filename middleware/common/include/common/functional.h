//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <utility>
#include <functional>
#include <memory>

namespace casual
{
   namespace common
   {

      namespace detail
      {

         //!
         //! memberfunction
         //!
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

         //!
         //! free function/functor
         //!
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

      //!
      //!
      //! @note "inspired" by cppreference.com 
      //! 
      template< typename C, typename Tuple>
      constexpr decltype( auto) apply( C&& callable, Tuple&& t)
      {
         return detail::apply_implementation(
            std::forward< C>( callable), std::forward<Tuple>( t),
            std::make_index_sequence< std::tuple_size< std::remove_reference_t< Tuple>>::value>{});
      }

      namespace movable
      {
         template< typename F> struct function;

         template< class R, class... Args>
         struct function< R( Args...)>
         {

            //template< typename F>
            //function( F callable) : m_callable( std::make_unique< basic_holder< F>>( std::move( callable))) {}

            template< typename F>
            function( F&& callable) : m_callable( std::make_unique< basic_holder< std::decay_t<F>>>( std::move( callable))) {}

            function( function&&) noexcept = default;
            function& operator = ( function&&) noexcept = default;

            R operator () ( Args... args) const 
            { 
               return m_callable->invoke( std::forward< Args>( args)...);
            }
         private:

            struct base_holder 
            {
               virtual R invoke( Args... args) = 0;
            };

            template< typename Callable>
            struct basic_holder final : base_holder 
            {
               basic_holder( Callable&& callable) : m_callable( std::move( callable)) {}

               R invoke( Args... args) override
               {
                  return common::invoke( m_callable, std::forward< Args>( args)...);
               }
               Callable m_callable;
            };

            std::unique_ptr< base_holder> m_callable;
         };
/*
         template< typename R, typename... Args>
         R function< R(Args...)>::operator() ( Args... args) const
         {
            return m_callable->invoke( std::forward< Args>( args)...);
         }
*/
      } // movable

   } // common

} // casual


