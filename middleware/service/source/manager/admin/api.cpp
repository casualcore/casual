//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "service/manager/admin/api.h"
#include "service/manager/admin/server.h"

#include "service/common.h"

#include "sf/service/protocol/call.h"

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
               inline namespace v1
               {
                  StateVO state()
                  {
                     Trace trace{ "service::manager::admin::api::state"};

                     sf::service::protocol::binary::Call call;

                     auto reply = call( service::name::state());

                     StateVO result;
                     reply >> CASUAL_MAKE_NVP( result);

                     return result;
                  }
                  
               } // v1

            } // api
         } // admin
      } // manager  
   } // service
} // casual