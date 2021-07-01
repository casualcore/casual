//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <system_error>

namespace casual
{
   namespace http
   {
      // we'll add the codes we find useful.
      enum struct code
      {
         // 2xx
         ok = 200,

         // 3xx

         // 4xx
         bad_request = 400,
         not_found = 404,
         request_timeout = 408,
         

         // 5xx
         internal_server_error = 500,

      };

      std::error_code make_error_code( http::code code);
      
   } // http
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::http::code> : true_type {};
}
