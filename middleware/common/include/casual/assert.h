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
      common::log::category::error.flush();
      std::terminate();
   }

   //! logs `context` to error and terminates
   template< typename P, typename... Ts>
   auto assertion( P&& predicate, Ts&&... context) noexcept
   {
      if( ! predicate)
         casual::terminate( std::forward< Ts>( context)...);

      return std::forward< P>( predicate);
   }

   namespace detail
   {
      void log( std::string_view function, std::string_view expression, std::string_view file, platform::size::type line) noexcept;
      inline bool terminate( std::string_view function, std::string_view expression, std::string_view file, platform::size::type line) noexcept
      { 
         log( function, expression, file, line);
         std::terminate();
         return true;
      }
   } // detail

} // casual

#ifndef CASUAL_COMPILE_NO_ASSERT
#define CASUAL_ASSERT(expression) ((void)(!(expression) && ::casual::detail::terminate( __func__, #expression, __FILE__, __LINE__)))
#else
#define CASUAL_ASSERT(x) ((void)sizeof(x))
#endif
