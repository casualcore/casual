//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "common/message/gateway.h"
#include "common/communication/instance.h"

#include <vector>
#include <string>

namespace casual
{
   namespace gateway::unittest
   {
      using namespace common::unittest;

      inline auto discover( std::vector< std::string> services, std::vector< std::string> queues)
      {
         common::message::gateway::domain::discover::Request request{ common::process::handle()};
         request.domain =  common::domain::identity();
         request.services = std::move( services);
         request.queues = std::move( queues);
         auto correlation = common::communication::device::blocking::send( common::communication::instance::outbound::gateway::manager::device(), request);
         common::message::gateway::domain::discover::accumulated::Reply reply;
         common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply, correlation);
         return reply;
      }
      
   } // gateway::unittest
} // casual