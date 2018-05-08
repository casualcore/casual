//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <utility>

namespace casual 
{
   namespace common 
   {
      namespace predicate 
      {
         namespace detail 
         {
            template< typename P>
            auto variadic_predicate( P&& predicate)
            {
               return [=]( auto&&... param){
                  return predicate( std::forward< decltype( param)>( param)...);
               };
            }
         } // detail 
         
         template< typename P>
         auto make_and( P&& predicate) { return detail::variadic_predicate( std::forward< P>( predicate));}

         template< typename P, typename... Ts>
         auto make_and( P&& predicate, Ts&&... ts)
         {
            return [=]( auto&&... param){
               return predicate( param...) && make_and( std::move( ts)...)( param...);
            };
         }

         
         template< typename P>
         auto make_or( P&& predicate) { return detail::variadic_predicate( std::forward< P>( predicate));}

         template< typename P, typename... Ts>
         auto make_or( P&& predicate, Ts&&... ts)
         {
            return [=]( auto&&... param){
               return predicate( param...) || make_or( std::move( ts)...)( param...);
            };
         }


         template< typename P>
         auto make_order( P&& predicate) { return detail::variadic_predicate( std::forward< P>( predicate));}

         template< typename P, typename... Ts>
         auto make_order( P&& predicate, Ts&&... ts)
         {
            return [=]( auto&& l, auto&& r){
               if( predicate( l, r))
                  return true;
               if( predicate( r, l))
                  return false;

               return make_order( std::move( ts)...)( l, r);
            };
         }

         template< typename P>
         auto make_nested( P&& predicate) { return detail::variadic_predicate( std::forward< P>( predicate));}

         template< typename P, typename... Ts>
         auto make_nested( P&& predicate, Ts&&... ts)
         {
            return [=]( auto&& value){
               return predicate( make_nested( std::move( ts)...)( std::forward< decltype( value)>( value)));
            };
         }


         //!
         //! reverse the call order of left and right to a predicate
         //!
         template< typename T>
         auto inverse( T&& functor)
         {
            return [functor]( auto&& l, auto&& r){ return functor( r, l);};
         }

         //!
         //! negates a predicate
         //!
         template< typename T>
         auto negate( T&& functor)
         {
            return [functor]( auto&&... param){ return ! functor( param...);};
         }

      } // predicate 
   } // common 
} // casual 