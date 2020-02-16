//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/value/optional.h"
#include "common/code/system.h"
#include "common/log/category.h"
#include "common/signal.h"

namespace casual
{
   namespace common
   {
      namespace posix
      {
         namespace detail
         {
            template< typename... Ts>
            void log( Ts&&... ts) noexcept
            {
               common::log::line( common::log::category::error, common::code::last::system::error(), " - ", std::forward< Ts>( ts)...);
            }

         } // detail
         namespace log
         {
            //! checks posix result, and logs if error
            template< typename... Ts>
            int result( int result, Ts&&... ts) noexcept
            {
               if( result == -1)
                  detail::log( std::forward< Ts>( ts)...);
               return result;
            }
         } // log

         //! checks posix result, and throws appropriate exception if error
         //! @returns value of result, if no errors detected 
         int result( int result);
         
         namespace error
         {
            //! checks posix result, - if 'error' - log the error and throws appropriate exception
            //! @returns value of result, if no errors detected 
            template< typename... Ts>
            int result( int result, Ts&&... ts) noexcept
            {
               if( result == -1)
                  detail::log( std::forward< Ts>( ts)...);
               return posix::result( result);
            }

            constexpr auto no_error = static_cast< code::system>( 0);
            using optional_error = value::Optional< code::system, no_error>;

            //! @return an optional error code if there are a posix-error.
            optional_error optional( int result) noexcept;
         } // error

      } // posix
   } // common
} // casual