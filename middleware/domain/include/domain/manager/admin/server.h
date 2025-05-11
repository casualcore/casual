//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "domain/manager/admin/service/name.h"

#include "casual/manager/service.h"
#include "casual/manager/service/policy.h"


#include <vector>
#include <string>

namespace casual
{
   namespace domain::manager
   {
      struct State;

      namespace admin
      {
         std::vector< casual::manager::Service> services( manager::State& state);

         struct Policy : casual::manager::service::policy::Default
         {
            static void send_ack( const common::message::service::call::ACK& ack);

            static void initialize( const casual::manager::service::context::State& services);
         }; 

      } // admin

   } // domain::manager
} // casual


