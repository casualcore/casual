//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/message/dispatch.h"

namespace casual
{
   namespace common::message::dispatch
   {
      namespace condition::detail::handle
      {
         std::optional< std::system_error> error()
         {
            Trace trace{ "common::message::dispatch::condition::detail::handle::error"};
                  
            auto error = exception::capture();

            if( error.code() != code::casual::interrupted)
               return error;

            log::line( verbose::log, "pump interrupted");
            signal::dispatch();

            return {};
         }

      } // condition::detail::handle

   } // common::message::dispatch
   
} // casual