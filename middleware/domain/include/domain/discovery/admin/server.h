//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "casual/manager/service.h"

namespace casual
{
   namespace domain::discovery
   {
      struct State;
   }

   namespace domain::discovery::admin
   {
      namespace service::name
      {
         constexpr std::string_view state = ".casual/discovery/state";

      } // service::name

      std::vector< casual::manager::Service> services( discovery::State& state);

   } // domain::discovery::admin
   
} // casual


