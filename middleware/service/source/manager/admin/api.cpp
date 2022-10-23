//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "service/manager/admin/api.h"
#include "service/manager/admin/server.h"

#include "service/common.h"

#include "serviceframework/service/protocol/call.h"

namespace casual
{
   namespace service::manager::admin::api
   {
      model::State state()
      {
         Trace trace{ "service::manager::admin::api::state"};

         return serviceframework::service::protocol::binary::Call{}( service::name::state).extract< model::State>();
      }

   } // service::manager::admin::api
} // casual