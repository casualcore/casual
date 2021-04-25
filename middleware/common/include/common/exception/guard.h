//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/traits.h"
#include "common/log.h"
#include "common/exception/handle.h"
#include "common/code/casual.h"

#include <iosfwd>
#include <system_error>

namespace casual
{
   namespace common
   {
      namespace exception
      {
         namespace main
         {
            namespace log
            {
               namespace detail
               {
                  int void_return( common::function< void()> callable);
                  int int_return( common::function< int()> callable);
               }
               
               template< typename C>
               int guard( C&& callable)
               {
                  if constexpr( std::is_same_v< decltype( callable()), void>)
                     return detail::void_return( std::forward< C>( callable));
                  else
                     return detail::int_return( std::forward< C>( callable));
               }
            } // log

            namespace cli
            {
               int guard( common::function< void()> callable);
            } // cli
         } // main

         namespace detail
         {
            template< typename F, typename B> 
            auto guard( F&& callable, B&& fallback, traits::priority::tag< 1>) 
               -> decltype( common::traits::convertable::type( callable(), std::forward< B>( fallback)))
            {
               try 
               {
                  return callable();
               }
               catch( ...)
               {
                  log::line( log::category::error, exception::capture());
               }
               return std::forward< B>( fallback);
            }

            template< typename F, typename B> 
            auto guard( F&& callable, B&& fallback, traits::priority::tag< 0>) 
               -> decltype( common::traits::convertable::type( callable(), fallback()))
            {
               try 
               {
                  return callable();
               }
               catch( ...)
               {
                  log::line( log::category::error, exception::capture());
               }
               return fallback();
            }
            
         } // detail

         template< typename F, typename B> 
         auto guard( F&& callable, B&& fallback) 
            -> decltype( detail::guard( std::forward< F>( callable), std::forward< B>( fallback), traits::priority::tag< 1>{}))
         {
            return detail::guard( std::forward< F>( callable), std::forward< B>( fallback), traits::priority::tag< 1>{});
         }

         void guard( common::function< void()> callable);

      } // exception
   } // common
} // casual




