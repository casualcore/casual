//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/traits.h"

#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace exception
      {
         //! throws pending exception, catches it and log based on the exception. returns the 
         //! corresponding error code.
         int handle() noexcept;

         int handle( std::ostream& out) noexcept;

         template< typename F> 
         auto guard( F&& callable)
         {
            try 
            {
               callable();
               return 0;
            }
            catch( ...)
            {
               return exception::handle();
            }
         }

         template< typename F> 
         auto guard( std::ostream& out, F&& callable)
         {
            try 
            {
               callable();
               return 0;
            }
            catch( ...)
            {
               return exception::handle( out);
            }
         }

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
                  exception::handle();
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
                  exception::handle();
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

      } // exception
   } // common
} // casual




