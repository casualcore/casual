//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "casual/manager/service.h"
#include "casual/manager/service/policy.h"


#include <vector>
#include <string>

namespace casual
{
   namespace service::manager
   {
      struct State;

      namespace admin
      {
         namespace service::name
         {
            constexpr auto state = ".casual/service/state";

            namespace metric
            {
               constexpr auto reset = ".casual/service/metric/reset";
            } // metric

         } // service::name

         std::vector< casual::manager::Service> services( manager::State& state);

         //! service-manager needs to have it's own policy for casual::manager::context, since
         //! we can't communicate with blocking to the same ipc-device (with read, who is
         //! going to write? with write, what if the ipc-device is full?)
         struct Policy : casual::manager::service::policy::Default
         {
            static void send_ack( const common::message::service::call::ACK& ack);

            static void initialize( const casual::manager::service::context::State& services, manager::State& state);
         }; 

      } // admin

   } // service::manager
} // casual

