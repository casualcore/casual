//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "service/manager/admin/model.h"

#include <vector>
#include <string>

namespace casual
{
   namespace gateway::unittest
   {
      using namespace common::unittest;

      void discover( std::vector< std::string> services, std::vector< std::string> queues);

      namespace service
      {
         casual::service::manager::admin::model::State state();
      } // service
      
   } // gateway::unittest
} // casual