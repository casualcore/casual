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
   namespace gateway
   {
      namespace transform
      {

         manager::State state( configuration::model::gateway::Model configuration);

         manager::admin::model::State state( const manager::State& state, 
            std::vector< message::reverse::inbound::state::Reply> inbounds, 
            std::vector< message::reverse::outbound::state::Reply> outbounds);

      } // transform
   } // gateway
} // casual

