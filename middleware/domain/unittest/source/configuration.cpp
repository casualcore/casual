//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/unittest/configuration.h"
#include "domain/common.h"
#include "domain/manager/admin/server.h"
#include "domain/manager/admin/call.h"


#include "common/message/dispatch/handle.h"
#include "common/message/dispatch.h"


namespace casual
{
   using namespace common;

   namespace domain::unittest::configuration
   {

      casual::configuration::user::Model get()
      {
         Trace trace{ "domain::unittest::configuration::get"};

         return manager::admin::call::service< casual::configuration::user::Model>( manager::admin::service::name::configuration::get);
      }

      casual::configuration::user::Model post( casual::configuration::user::Model wanted)
      {
         Trace trace{ "domain::unittest::configuration::post"};

         manager::admin::call::service( manager::admin::service::name::configuration::post, wanted);
         return configuration::get();
      }

      casual::configuration::user::Model put( casual::configuration::user::Model wanted)
      {
         Trace trace{ "domain::unittest::configuration::put"};

         manager::admin::call::service( manager::admin::service::name::configuration::put, wanted);
         return configuration::get();
      }

   } // domain::unittest::configuration
} // casual
