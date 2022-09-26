//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

namespace casual
{
   namespace common
   {
      template< typename T>
      void sink( T&& value)
      {
         [[maybe_unused]] auto sinked = std::move( value);
      }
   } // common
} // casual