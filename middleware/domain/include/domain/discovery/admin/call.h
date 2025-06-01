//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/discovery/instance.h"

#include "casual/manager/service/call.h"


namespace casual
{
   namespace domain::discovery::admin::call
   {

      template< typename R, typename... Ts>
      R service( std::string_view service, Ts&&... arguments)
      {
         return casual::manager::service::call< R>( instance::device(), service, std::forward< Ts>( arguments)...);
      }
      
   } // domain::manager::call
   
} // casual
