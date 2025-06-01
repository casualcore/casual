//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "file/message.h"
#include "file/manager/state.h"

#include "common/message/transaction.h"

namespace casual::file::resource
{
   using State = file::manager::State;
   using Request = file::manager::State::Request;

   using Reserve = file::message::reserve::Request;
   using Reply = file::message::reserve::Reply;

   using Prepare = common::message::transaction::resource::prepare::Request;
   using Commit = common::message::transaction::resource::commit::Request;
   using Rollback = common::message::transaction::resource::rollback::Request;
   using Exit = common::message::event::process::Exit;

   using Shutdown = common::message::shutdown::Request;

   void reserve( State& state, const Reserve& request);
   void prepare( State& state, const Prepare& request);
   void commit( State& state, const Commit& request);
   void rollback( State& state, const Rollback& request);
   void mitigate( State& state, const Exit& request);
   void shutdown( State& state, const Shutdown& request);

   namespace recovery
   {
      std::vector< common::transaction::global::ID> commit( State& state, std::vector< common::transaction::global::ID> gtrids);
      std::vector< common::transaction::global::ID> rollback( State& state, std::vector< common::transaction::global::ID> gtrids);
   } // recovery

} // casual::file::resource