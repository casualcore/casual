//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/stream.h"

#include <string>
#include <sstream>

namespace casual
{
   namespace common::string
   {
      //! composes a string from several parts, using the stream operator
      template< typename... Parts>
      inline std::string compose( Parts&&... parts)
      {
         std::ostringstream out;
         stream::write( out, std::forward< Parts>( parts)...);
         return std::move( out).str();
      }
   } // common::string
} // casual