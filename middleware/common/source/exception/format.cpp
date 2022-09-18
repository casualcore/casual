//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/exception/format.h"

#include "common/log.h"
#include "common/terminal.h"

namespace casual
{
   namespace common::exception::format
   {
      void terminal( std::ostream& out, const std::system_error& error)
      {
         stream::write( out, terminal::color::value::red, error.code(), terminal::color::value::no_color, " ", error.what(), '\n');
      }

   } // common::exception::format
} // casual