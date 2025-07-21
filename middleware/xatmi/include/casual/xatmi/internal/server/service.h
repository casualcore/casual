//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/xatmi/defines.h"

#include "server/service.h"

#include "common/service/type.h"

#include <functional>

namespace casual
{
   namespace xatmi::internal::server::service
   {
      using function_type = std::function< void( TPSVCINFO*)>;

      casual::server::Service create( std::string name, function_type function, common::service::transaction::Type transaction, common::service::visibility::Type visibility, std::string category);
      casual::server::Service create( std::string name, function_type function);
      
   } // xatmi::internal::server::service
   
} // casual
