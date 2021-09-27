//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/unittest/utility.h"

#include "domain/discovery/api.h"

#include "serviceframework/service/protocol/call.h"
#include "service/manager/admin/server.h"

#include "common/communication/ipc.h"

namespace casual
{
   using namespace common;
   namespace gateway::unittest
   {
      using namespace common::unittest;

      void discover( std::vector< std::string> services, std::vector< std::string> queues)
      {
         casual::domain::discovery::external::Request request{ common::process::handle()};
         request.content.services = std::move( services);
         request.content.queues = std::move( queues);
         if( auto correlation = casual::domain::discovery::external::request( request))
         {
            casual::domain::discovery::external::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply, correlation);
         }
      }

      namespace service
      {
         casual::service::manager::admin::model::State state()
         {
            serviceframework::service::protocol::binary::Call call;
            auto reply = call( casual::service::manager::admin::service::name::state());
            return reply.extract< casual::service::manager::admin::model::State>();

         }
      } // service
      
   } // gateway::unittest
   
} // casual