//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/manager/service.h"

namespace casual
{
   namespace file::manager
   {
      struct State;

      namespace admin
      {
         std::vector< casual::manager::Service> services( manager::State& state);
      } // admin
   } // manager

} // casual


