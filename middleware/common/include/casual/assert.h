//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/log/category.h"

#include "common/code/casual.h"

namespace casual
{
   //! logs `context` to error and terminates
   template< typename... Ts>
   [[noreturn]] void terminate( Ts&&... context) noexcept
   {
      common::log::line( common::log::category::error, common::code::casual::fatal_terminate, ' ', std::forward< Ts>( context)...);
      std::terminate();
   }

} // casual