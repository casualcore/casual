//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "domain/discovery/admin/service/name.h"

#include "casual/manager/service.h"

namespace casual
{
   namespace domain::discovery
   {
      struct State;
   }

   namespace domain::discovery::admin
   {
      std::vector< casual::manager::Service> services( discovery::State& state);

   } // domain::discovery::admin
   
} // casual


