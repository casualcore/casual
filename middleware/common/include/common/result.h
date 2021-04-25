//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/code/convert.h"
#include "common/code/system.h"
#include "common/code/raise.h"
#include "common/algorithm/compare.h"

#include <system_error>
#include <string>

namespace casual
{
   namespace common::posix
   {
         namespace detail
         {
            inline bool predicate( int result) { return result != -1;}
            inline bool predicate( const void* result) { return result != nullptr;}
         } // detail

         //! checks posix result, if -1 or nullptr -> transform `errno` to `code::casual` and throws this.
         //! 
         //! @returns value of result, if no errors detected 
         template< typename R, typename... Ts>
         auto result( R result, Ts&&... ts)
         {
            if( detail::predicate( result))
               return result;

            auto code = code::system::last::error();
            code::raise::error( code::convert::to::casual( code), std::forward< Ts>( ts)..., " - errno: ", code);
         }

         //! checks posix result, if -1 or nullptr 
         //! if errno is one of the provided codes, `alternative` is returned. 
         /// if not, errno is transformed  to `code::casual` and throwned
         template< typename R, typename A, typename... Codes>
         auto alternative( R result, A alternative, Codes&&... codes) -> std::common_type_t< R, A>
         {
            if( detail::predicate( result))
               return result;

            auto code = code::system::last::error();

            if( algorithm::compare::any( code, codes...))
               return alternative;

            code::raise::error( code::convert::to::casual( code), " - errno: ", code);
         }

         namespace log
         {
            //! checks posix result, and logs if error
            //! @return true if posix result is 'ok', false otherwise
            template< typename R, typename... Ts>
            bool result( R result, Ts&&... ts)
            {
               if( detail::predicate( result))
                  return true;

               auto system = code::system::last::error();
               auto code = code::convert::to::casual( system);
               common::log::line( code::stream( code), code, ' ', std::forward< Ts>( ts)...,  " - errno: ", system);
               return false;
            }

         } // log

         //! check result and if error returns last system error (errno/errc)
         //! otherwise std::error_condition{}
         template< typename R>
         std::error_condition error( R result)
         {
            if( detail::predicate( result))
               return {};

            return { code::system::last::error()};
         }

   } // common::posix
} // casual