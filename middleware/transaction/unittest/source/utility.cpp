//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/unittest/utility.h"
#include "transaction/manager/admin/server.h"

#include "serviceframework/service/protocol/call.h"

#include "common/message/transaction.h"
#include "common/communication/instance.h"

namespace casual
{
   namespace transaction::unittest
   {
      using namespace common;

      common::code::tx commit( const common::transaction::ID& trid)
      {
         common::message::transaction::commit::Request request{ process::handle()};
         request.trid = trid;

         auto reply = communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request);
         return reply.state;
      }

      manager::admin::model::State state()
      {
         common::unittest::service::wait::until::advertised( manager::admin::service::name::state);
         serviceframework::service::protocol::binary::Call call;
         return call( manager::admin::service::name::state).extract< manager::admin::model::State>();
      }
      
   } // transaction::unittest
   
} // casual