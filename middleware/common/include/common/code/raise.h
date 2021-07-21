//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/code/log.h"

#include "common/log/category.h"
#include "common/log.h"

#include <system_error>


namespace casual
{
   namespace common::code::raise
   {
      namespace detail::string
      {
         //! composes a string from several parts, using the stream operator
         template< typename... Parts>
         inline std::string compose( Parts&&... parts)
         {
            std::ostringstream out;
            stream::write( out, std::forward< Parts>( parts)...);
            return std::move( out).str();
         }
      } // detail::string

      template< typename Code, typename... Ts>
      [[noreturn]] void error( Code code, Ts&&... ts) noexcept( false)
      {
         throw std::system_error( std::error_code{ code}, detail::string::compose( std::forward< Ts>( ts)...));
      }

   } // common::code::raise
} // casual