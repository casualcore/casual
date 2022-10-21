//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "queue/manager/state.h"
#include "queue/manager/admin/model.h"

#include "queue/common/ipc/message.h"

#include "configuration/model.h"

namespace casual
{
   namespace queue::manager::transform
   {
      namespace model
      {
         admin::model::State state(
            const manager::State& state,
            std::vector< ipc::message::group::state::Reply> groups,
            std::vector< ipc::message::forward::group::state::Reply> forwards);

         namespace message
         {
            std::vector< admin::model::Message> meta( std::vector< ipc::message::group::message::meta::Reply> messages);
            
         } // message 

      } // model

      casual::configuration::model::queue::Model configuration( const State& state);

   } // queue::manager::transform
} // casual