//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "gateway/manager/state.h"

#include "gateway/manager/admin/model.h"
#include "configuration/model.h"


namespace casual
{
   namespace gateway::manager::transform
   {
      manager::admin::model::State state( const manager::State& state, 
         std::tuple< std::vector< message::inbound::state::Reply>, std::vector< message::inbound::reverse::state::Reply>> inbounds, 
         std::tuple< std::vector< message::outbound::state::Reply>, std::vector< message::outbound::reverse::state::Reply>> outbounds);

      casual::configuration::model::gateway::Model configuration( const manager::State& state);

   } // gateway::manager::transform
} // casual

