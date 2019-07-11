//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <system_error>

namespace casual
{
   namespace queue
   {
      enum class code : int
      {
         ok = 0, 
         no_message = 1,
         no_queue = 10,
         argument = 20,
         system = 30,
      };

      std::error_code make_error_code( queue::code code);


   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::queue::code> : true_type {};
}
