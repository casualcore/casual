//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <system_error>

namespace casual
{
   namespace common
   {
      namespace exception
      {

         //! catches all possible exceptions and convert them to system_error
         std::system_error capture() noexcept;

         void sink() noexcept;

      } // exception
   } // common
} // casual