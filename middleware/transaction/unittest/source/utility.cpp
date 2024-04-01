//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/unittest/utility.h"
#include "transaction/manager/admin/server.h"

#include "serviceframework/service/protocol/call.h"

#include "common/unittest.h"

namespace casual
{
   namespace transaction::unittest
   {
      manager::admin::model::State state()
      {
         common::unittest::service::wait::until::advertised( manager::admin::service::name::state);
         serviceframework::service::protocol::binary::Call call;
         return call( manager::admin::service::name::state).extract< manager::admin::model::State>();
      }
      
   } // transaction::unittest
   
} // casual