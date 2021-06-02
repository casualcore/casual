//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/unittest/configuration.h"
#include "domain/common.h"
#include "domain/manager/admin/server.h"

#include "serviceframework/service/protocol/call.h"


namespace casual
{
   namespace domain::manager::unittest::configuration
   {

      casual::configuration::user::Domain get()
      {
         Trace trace{ "domain::manager::unittest::configuration::get"};

         serviceframework::service::protocol::binary::Call call;
         auto reply = call( admin::service::name::configuration::get);
         casual::configuration::user::Domain result;
         reply >> result;
         return result;
      }

   } // domain::manager::unittest::configuration
} // casual