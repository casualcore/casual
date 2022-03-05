//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/stream.h"

#include <system_error>

#include <signal.h>

namespace casual
{
   namespace common::code
   {

      enum class signal : platform::signal::native::type
      {
         absent = 0,
         alarm = SIGALRM,
         interrupt = SIGINT,
         kill = SIGKILL,
         quit = SIGQUIT,
         child = SIGCHLD,
         terminate = SIGTERM,
         user = SIGUSR1,
         pipe = SIGPIPE,
         hangup = SIGHUP,
      };
      std::string_view description( code::signal code) noexcept;

      std::error_code make_error_code( code::signal code);
      
   } // common::code
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::signal> : true_type {};
}


