//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include <system_error>

namespace casual
{
   namespace common::code
   {
      enum class queue : int
      {
         ok = 0, 
         no_message = 1,
         no_queue = 10,
         argument = 20,
         system = 30,
         signaled = 40,
      };
      std::string_view description( code::queue value) noexcept;

      std::error_code make_error_code( code::queue code);

   } // queue
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::queue> : true_type {};
}
