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
   namespace service
   {
      namespace manager
      {
         namespace admin
         {
            namespace api
            {
               model::State state()
               {
                  Trace trace{ "service::manager::admin::api::state"};

                  serviceframework::service::protocol::binary::Call call;

                  auto reply = call( service::name::state());

                  model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }

            } // api
         } // admin
      } // manager  
   } // service
} // casual