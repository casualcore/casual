//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/stream.h"
#include "common/signal.h"

#include <system_error>

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
            validation,
            invalid_configuration,
            invalid_document,
            invalid_node,
            invalid_version,
         };


         std::error_code make_error_code( casual code);

         
         log::Stream& stream( casual code);

      } // code
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::casual> : true_type {};
}

