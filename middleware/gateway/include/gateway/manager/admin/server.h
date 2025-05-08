//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/manager/service.h"

#include <vector>

namespace casual
{
   namespace gateway::manager
   {
      struct State;

      namespace admin
      {
         namespace service
         {
            namespace name
            {
               constexpr auto state = ".casual/gateway/state";
            } // name
         } // service

         std::vector< casual::manager::Service> services( manager::State& state);

      } // admin

   } // gateway::manager

} // casual


