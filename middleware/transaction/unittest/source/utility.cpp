//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/unittest/utility.h"
#include "transaction/manager/admin/server.h"

#include "serviceframework/service/protocol/call.h"

namespace casual
{
   namespace transaction::unittest
   {
      manager::admin::model::State state()
      {
         serviceframework::service::protocol::binary::Call call;
         return call( manager::admin::service::name::state()).extract< manager::admin::model::State>( "result");
      }
      
   } // transaction::unittest
   
} // casual