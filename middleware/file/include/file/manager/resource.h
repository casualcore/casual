//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/transaction/context.h"

#include "file/message.h"
#include "file/manager/state.h"

namespace casual::file::resource
{
   using State    = file::manager::State;

   using Reserve  = file::message::reserve::Request;
   using Reply    = file::message::reserve::Reply;

   using Commit   = common::message::transaction::resource::commit::Request;
   using Rollback = common::message::transaction::resource::rollback::Request;
   using Exit     = common::message::event::process::Exit;

   using Shutdown = common::message::shutdown::Request;

   void reserve(  State& state, Reserve request);
   void commit(   State& state, const Commit& request);
   void rollback( State& state, const Rollback& request);
   void mitigate( State& state, const Exit& request);
   void shutdown( State& state, const Shutdown& request);
} // casual::file::resource