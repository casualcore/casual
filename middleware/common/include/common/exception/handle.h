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



         //! catches all possible exceptions and convert them to system_error
         std::system_error error() noexcept;


         //! tries to catch all possible exceptions, including error_code/conditions 
         //! logs with appropriate stream, and returns the corresponding system_error
         //std::system_error handle();

         namespace detail
         {
            template< typename... Ts>
            std::system_error handle( std::ostream& out, Ts&&... ts)
            {
               auto error = exception::error();
               log::line( out, error, std::forward< Ts>( ts)...);
               return error;
            }
         } // detail
         
         inline std::system_error handle( std::ostream& out)
         {
            return detail::handle( out);
         }

         template< typename... Ts>
         std::system_error handle( std::ostream& out, Ts&&... ts)
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