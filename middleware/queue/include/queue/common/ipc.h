//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/instance.h"
#include "common/communication/ipc/flush/send.h"

namespace casual
{
   namespace queue::ipc
   {

      inline auto& device() { return common::communication::ipc::inbound::device();}

      namespace queue
      {
         inline auto& manager() { return common::communication::instance::outbound::queue::manager::device();}   
      } // queue

      namespace service
      {
         inline auto& manager() { return common::communication::instance::outbound::service::manager::device();}
      } // service

      namespace transaction
      {
         inline auto& manager() { return common::communication::instance::outbound::transaction::manager::device();}
      } // transaction

      namespace gateway::optional
      {
         inline auto& manager() { return common::communication::instance::outbound::gateway::manager::optional::device();}
      } // gateway::optional


      //! since we're sending a lot of messages in the same 'batch', we need to flush our inbound
      //! to mitigate 'deadlocks' 
      namespace flush
      {
         using namespace common::communication::ipc::flush;
      } // flush

   } // queue::ipc
} // casual