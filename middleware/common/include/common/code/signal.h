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
         
         enum class signal : int
         {
            //ok = 0,
            alarm = cast::underlying( common::signal::Type::alarm),
            interupt = cast::underlying( common::signal::Type::interrupt),
            kill = cast::underlying( common::signal::Type::kill),
            quit = cast::underlying( common::signal::Type::quit),
            child = cast::underlying( common::signal::Type::child),
            terminate = cast::underlying( common::signal::Type::terminate),
            user = cast::underlying( common::signal::Type::user),
            pipe = cast::underlying( common::signal::Type::pipe),
         };


         std::error_code make_error_code( signal code);
         common::log::Stream& stream( signal code);


      } // code
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::signal> : true_type {};
}


