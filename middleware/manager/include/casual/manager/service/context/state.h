//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/manager/service.h"

#include "common/serialize/macro.h"

#include <string>
#include <unordered_map>

namespace casual
{
   namespace manager::service::context
   {
      struct State
      {
         State() = default;
         State( std::vector< manager::Service> services);

         std::unordered_map< std::string, manager::Service> services;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( services);
         )
      };

   } // manager::service::context
} // casual
