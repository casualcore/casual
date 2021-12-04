//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/unittest/discover.h"
#include "domain/manager/admin/server.h"
#include "domain/common.h"
#include "domain/discovery/api.h"

#include "serviceframework/service/protocol/call.h"
#include "common/communication/ipc.h"

namespace casual
{
   using namespace common;
   namespace domain::manager::unittest
   {
      void discover( std::vector< std::string> services, std::vector< std::string> queues)
      {
         Trace trace{ "domain::unittest::discover"};
         casual::domain::discovery::external::Request request{ common::process::handle()};
         request.content.services = std::move( services);
         request.content.queues = std::move( queues);
         if( auto correlation = casual::domain::discovery::external::request( request))
         {
            casual::domain::discovery::external::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply, correlation);
         }
      }

      
   } // domain::manager::unittest
   
} // casual