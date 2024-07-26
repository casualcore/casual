//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/manager/state.h"

#include "configuration/model.h"

namespace casual
{
   namespace gateway::manager::configuration
   {
      void conform( State& state, casual::configuration::Model wanted);

      namespace add
      {
         void group( State& state, casual::configuration::model::gateway::outbound::Group configuration);
         void group( State& state, casual::configuration::model::gateway::inbound::Group configuration);
      } // add

      
   } // gateway::manager::configuration
} // casual