//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/unittest/utility.h"
#include "gateway/manager/admin/server.h"

#include "serviceframework/service/protocol/call.h"


#include "common/communication/ipc.h"

namespace casual
{
   using namespace common;
   namespace gateway::unittest
   {
      manager::admin::model::State state()
      {
         serviceframework::service::protocol::binary::Call call;
         auto reply = call( manager::admin::service::name::state);
         return reply.extract< manager::admin::model::State>();
      }

      
   } // gateway::unittest
   
} // casual