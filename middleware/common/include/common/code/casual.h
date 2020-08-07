//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <system_error>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace code
      {
         enum class casual : int
         {
            //ok = 0,
            shutdown = 1,
            interupted,

            invalid_configuration,
            invalid_document,
            invalid_node,
            invalid_version,
            invalid_path,
            invalid_argument,
            invalid_semantics,

            failed_transcoding,

            communication_unavailable,
            communication_refused,
            communication_protocol,
            communication_retry,
            communication_no_message,

            domain_unavailable,
            domain_running,
            domain_instance_unavailable,

            buffer_type_duplicate,

            internal_out_of_bounds,
            internal_unexpected_value,
            internal_correlation,
         };

         std::error_code make_error_code( code::casual code);
         const char* description( code::casual code);

      } // code
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::casual> : true_type {};
}

