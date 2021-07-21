//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/user.h"

#include "common/strong/id.h"

namespace casual
{
   namespace domain::manager::unittest::configuration
   {
      casual::configuration::user::Domain get();
      
      //! post the `wanted` configuration, and waits for it to apply, then calls `get()` and return
      //! the updated configuration state
      casual::configuration::user::Domain post( casual::configuration::user::Domain wanted);


   } // domain::manager::unittest::configuration
} // casual