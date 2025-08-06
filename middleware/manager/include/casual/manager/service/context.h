//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/manager/service.h"
#include "casual/manager/service/context/state.h"
#include "casual/manager/service/handle.h"

#include "common/serialize/macro.h"
#include "common/message/dispatch.h"
#include "common/message/domain.h"

#include <unordered_map>

namespace casual
{
   namespace manager::service
   {
      namespace context
      {
         using dispatch_type = common::message::dispatch::basic_handler< common::communication::ipc::message::Complete>;

      } // context

      template< typename Policy>
      struct Context 
      {         
         template< typename... Ts>
         context::dispatch_type initialize( std::vector< manager::Service> services, Ts&&... ts) &
         {
            m_state = context::State{ std::move( services)};

            Policy::initialize( m_state, std::forward< Ts>( ts)...);

            return context::dispatch_type{ 
               handle::call< Policy>( m_state)
            };
         }

         template< typename... Ts>
         void advertise( const common::message::domain::process::lookup::Reply& message, Ts&&... ts) &
         {            
            Policy::advertise( m_state, message, std::forward< Ts>( ts)...);
         }

         auto advertise() const
         {
            return service::advertise::transform( m_state.services | std::views::values);
         }

         auto& services() const
         {
            return m_state.services;
         }

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_state);
         )

      private:
         context::State m_state;
      };
      
   } // manager::service
} // casual