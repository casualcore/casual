//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "transaction/manager/admin/service/name.h"

#include "casual/manager/service.h"

#include <vector>

namespace casual
{
   namespace transaction::manager
   {
      struct State;
      namespace admin
      {
         std::vector< casual::manager::Service> services( manager::State& state);
      } // admin

   } // transaction::manager
} // casual

