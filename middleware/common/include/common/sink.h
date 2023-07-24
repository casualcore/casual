//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <type_traits>

namespace casual
{
   namespace common
   {
      template< typename T>
      void sink( T&& value) requires std::is_rvalue_reference_v< decltype( value)>
      {
         [[maybe_unused]] auto sinked = std::forward< T>( value);
      }
   } // common
} // casual