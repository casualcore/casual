//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/ipc.h"

#include "domain/manager/state.h"
#include "domain/manager/handle.h"
#include "domain/pending/message/message.h"
#include "domain/pending/message/instance.h"

namespace casual
{
   using namespace common;

   namespace domain::manager::ipc
   {
      communication::ipc::inbound::Device& device()
      {
         return communication::ipc::inbound::device(); 
      }

      namespace pending
      {
         void send( const State& state, common::message::pending::Message&& pending)
         {
            communication::device::blocking::send(
               state.singleton( casual::domain::pending::message::instance::identity.id).ipc,
               casual::domain::pending::message::Request{ std::move( pending)});
         }

      } // pending

      void send( const State& state, common::message::pending::Message&& pending)
      {
         if( ! message::pending::non::blocking::send( pending))
            ipc::pending::send( state, std::move( pending));
      }

   } // domain::manager::ipc
} // casual