//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/exception/compose.h"

#include <system_error>

namespace casual
{
   namespace common::code::raise
   {
      template< typename Code, typename... Ts>
      [[noreturn]] void error( Code code, Ts&&... ts) noexcept( false)
      {
         throw exception::compose( code, std::forward< Ts>( ts)...);
      }

   } // common::code::raise
} // casual