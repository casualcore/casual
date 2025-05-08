//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/manager/service.h"
#include "casual/manager/service/context.h"
#include "casual/manager/service/handle.h"

#include "common/message/service.h"
#include "common/message/domain.h"

namespace casual
{
   namespace manager::service::policy
   {
      struct Default
      {
         //! sends a ACK to SM
         static void send_ack( const common::message::service::call::ACK& ack);

         //! sends a process lookup for SM
         static void initialize( const context::State& state);

         //! @returns a message::service::Advertise with the current process information
         static common::message::service::Advertise advertise( const context::State& state);

         //! advertise the services if the lookup indicates that SM is online
         static void advertise( const context::State& state, const common::message::domain::process::lookup::Reply& message);
        
      };
      
   } // manager::service::policy
} // casual