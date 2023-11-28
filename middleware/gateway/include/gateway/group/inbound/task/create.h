//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/inbound/state.h"

#include "common/message/service.h"
#include "queue/common/ipc/message.h"

#include "casual/task/concurrent.h"

namespace casual
{
   namespace gateway::group::inbound::task::create
   {
      namespace lookup
      {
         enum struct Context
         {
            localized,
            forward
         };
         
      } // lookup

      namespace service
      {
         casual::task::concurrent::Unit call( State& state, lookup::Context context, common::message::service::call::callee::Request&& message);
         casual::task::concurrent::Unit conversation( State& state, lookup::Context context, common::message::conversation::connect::callee::Request&& message);
         
      } // service

      namespace queue
      {
         casual::task::concurrent::Unit enqueue( State& state, lookup::Context context, casual::queue::ipc::message::group::enqueue::Request&& message);
         casual::task::concurrent::Unit dequeue( State& state, lookup::Context context, casual::queue::ipc::message::group::dequeue::Request&& message);
      } // queue

      casual::task::concurrent::Unit transaction( State& state, common::message::transaction::resource::prepare::Request&& message);
      casual::task::concurrent::Unit transaction( State& state, common::message::transaction::resource::commit::Request&& message);
      casual::task::concurrent::Unit transaction( State& state, common::message::transaction::resource::rollback::Request&& message);

   
      
   } // gateway::group::inbound::task::create   
} // casual
