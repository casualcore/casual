//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/outbound/state.h"

namespace casual
{
   namespace gateway::outbound::error::reply
   {
      //! Tries to send error replise to the in-flight messages (route-points)
      //! @param route 
      void point( const state::route::Point& point);
      void point( const state::route::service::Point& point); 

   } // gateway::outbound::error::reply
} // casual