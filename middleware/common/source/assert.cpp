//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/assert.h"

namespace casual
{
   
   namespace detail
   {
      void log( std::string_view function, std::string_view expression, std::string_view file, platform::size::type line) noexcept
      {
         common::log::line( common::log::category::error, common::code::casual::fatal_terminate, " assert failed: ", expression, " - ", function, " ", file, ":", line);
      }
      
   } // detail

} // casual