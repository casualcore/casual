//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace signal
         {
            template< message::Type type>
            using basic_signal = basic_message< type>;

            using Timeout = basic_signal< Type::signal_timeout>;
            using Hangup = basic_signal< Type::signal_hangup>;
         } // signal
      } // message
   } // common
} // casual