//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/unittest/utility.h"

#include "domain/manager/admin/server.h"

#include "serviceframework/service/protocol/call.h"

namespace casual
{
   using namespace common;
   namespace domain::unittest
   {
      manager::admin::model::State state()
      {
         serviceframework::service::protocol::binary::Call call;
         auto reply = call( manager::admin::service::name::state);
         return reply.extract< manager::admin::model::State>();
      }

   } // domain::unittest
} // casual
