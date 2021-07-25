//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "service/manager/state.h"

#include "configuration/model.h"

namespace casual
{
   namespace service::manager::configuration
   {
      void conform( State& state, casual::configuration::model::service::Model current, casual::configuration::model::service::Model wanted);
      
   } // service::manager::configuration
} // casual