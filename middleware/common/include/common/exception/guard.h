//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/traits.h"
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
            template< typename F> 
            int guard( F&& callable)
            {
               try 
               {
                  callable();
                  return 0;
               }
               catch( ...)
               {
                  auto error = exception::error();
                  if( error.code() == code::casual::shutdown)
                     return 0;
                  return error.code().value();
               }
            }

            template< typename F> 
            int guard( std::ostream& out, F&& callable)
            {
               try 
               {
                  callable();
                  return 0;
               }
               catch( ...)
               {
                  auto error = exception::handle( out);
                  if( error.code() == code::casual::shutdown)
                     return 0;
                  return error.code().value();
               }
            } 
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
                  exception::sink::silent();
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
                  exception::sink::silent();
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

         template< typename F> 
         void guard( F&& callable)
         {
            try 
            {
               callable();
            }
            catch( ...)
            {
               exception::sink::silent();
            }
         }

      } // exception
   } // common
} // casual




