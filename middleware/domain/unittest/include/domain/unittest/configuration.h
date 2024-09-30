//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/user.h"
#include "configuration/model.h"
#include "configuration/model/load.h"

#include "common/unittest/file.h"

namespace casual
{
   namespace domain::unittest::configuration
   {
      casual::configuration::user::Model get();
      
      //! post the `wanted` configuration, and waits for it to apply, then calls `get()` and return
      //! the updated configuration state
      casual::configuration::user::Model post( casual::configuration::user::Model wanted);

      casual::configuration::user::Model put( casual::configuration::user::Model wanted);

      namespace detail
      {
         casual::configuration::Model load( std::vector< std::string_view> contents);
      } // detail




   } // domain::unittest::configuration
} // casual