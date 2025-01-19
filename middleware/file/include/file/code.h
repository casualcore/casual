//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <system_error>

namespace casual
{
   namespace file
   {
      enum class code : int
      {
         ok = 0, 
         busy = 1,
         error = 2,
      };

      std::string_view description( code value) noexcept;

      std::error_code make_error_code( code value);

   } // file
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::file::code> : true_type {};
}
