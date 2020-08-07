//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/log/stream.h"

#include <system_error>
#include <iosfwd>
#include <string>

namespace casual
{
   namespace common
   {
      namespace exception
      {



         //! catches all possible exceptions and convert them to error_code
         std::error_code code() noexcept;


         //! tries to catch all possible exceptions, including error_code/conditions 
         //! logs with appropriate stream, and returns the corresponding error_code
         //std::error_code handle();

         namespace detail
         {
            template< typename... Ts>
            std::error_code handle( std::ostream& out, Ts&&... ts)
            {
               auto code = exception::code();
               log::line( out, code, std::forward< Ts>( ts)...);
               return code;
            }
         } // detail
         
         inline std::error_code handle( std::ostream& out)
         {
            return detail::handle( out);
         }

         template< typename... Ts>
         std::error_code handle( std::ostream& out, Ts&&... ts)
         {
            return detail::handle( out, ' ', std::forward< Ts>( ts)...);
         }


         namespace sink
         {
            void error() noexcept;
            void log() noexcept;
            void silent() noexcept;
         } // sink


      } // exception
   } // common
} // casual