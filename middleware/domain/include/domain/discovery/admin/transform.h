//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/discovery/admin/model.h"
#include "domain/discovery/state.h"

namespace casual
{
   namespace domain::discovery::admin
   {
      model::State transform( const State& state);

   } // domain::discovery::admin
} // casual
