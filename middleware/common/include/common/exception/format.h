//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <system_error>
#include <iosfwd>

namespace casual
{
   namespace common::exception::format
   {
      void terminal( std::ostream& out, const std::system_error& error);

   } // common::exception::format
} // casual