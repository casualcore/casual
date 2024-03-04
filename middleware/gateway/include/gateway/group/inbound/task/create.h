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

      using task_unit = casual::task::concurrent::Unit< common::strong::socket::id>;

      namespace service
      {
         [[nodiscard]] task_unit call( State& state, common::strong::socket::id descriptor, common::message::service::call::callee::Request&& message);
         [[nodiscard]] task_unit conversation( State& state, common::strong::socket::id descriptor, common::message::conversation::connect::callee::Request&& message);
         
      } // service

      namespace queue
      {
         [[nodiscard]] task_unit enqueue( State& state, common::strong::socket::id descriptor, casual::queue::ipc::message::group::enqueue::Request&& message);
         [[nodiscard]] task_unit dequeue( State& state, common::strong::socket::id descriptor, casual::queue::ipc::message::group::dequeue::Request&& message);
      } // queue

      [[nodiscard]] task_unit transaction( State& state, common::strong::socket::id descriptor, common::message::transaction::resource::prepare::Request&& message);
      [[nodiscard]] task_unit transaction( State& state, common::strong::socket::id descriptor, common::message::transaction::resource::commit::Request&& message);
      [[nodiscard]] task_unit transaction( State& state, common::strong::socket::id descriptor, common::message::transaction::resource::rollback::Request&& message);

   
      
   } // gateway::group::inbound::task::create   
} // casual
