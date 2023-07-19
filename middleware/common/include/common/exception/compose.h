//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/string/compose.h"

#include <system_error>

namespace casual
{
   namespace common::exception
   {
      template< typename Code, typename... Ts>
      auto compose( Code code, Ts&&... ts)
      {
         static_assert( std::is_error_code_enum_v< Code> || std::is_same_v< Code, std::errc>, "code has to be an error_code or errc enum");

         if constexpr( std::is_error_code_enum_v< Code>)
            return std::system_error{ std::error_code{ code}, string::compose( std::forward< Ts>( ts)...)};
         else
            return std::system_error{ std::to_underlying( code), std::system_category(), string::compose( std::forward< Ts>( ts)...)};
      }
   } // common::exception
} // casual