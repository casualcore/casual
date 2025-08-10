//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/manager/service/call.h"

#include "common/communication/instance.h"

namespace casual
{
   namespace domain::manager::admin::call
   {

      template< typename R, typename... Ts>
      R service( std::string_view service, Ts&&... arguments)
      {
         return casual::manager::service::call< R>( common::communication::instance::outbound::domain::manager::device(), service, std::forward< Ts>( arguments)...);
      }

      template< typename... Ts>
      void service( std::string_view service, Ts&&... arguments)
      {
         casual::manager::service::call( common::communication::instance::outbound::domain::manager::device(), service, std::forward< Ts>( arguments)...);
      }
      
   } // domain::manager::admin::call
   
} // casual
