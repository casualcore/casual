//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "queue/manager/manager.h"

#include "common/message/queue.h"
#include "common/message/transaction.h"
#include "common/message/gateway.h"
#include "common/message/domain.h"
#include "common/message/event.h"
#include "common/message/dispatch.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace queue
   {
      namespace manager
      {
         namespace ipc
         {
            common::communication::ipc::inbound::Device& device();
         } // ipc

         namespace handle
         {
            using dispatch_type = common::communication::ipc::dispatch::Handler;

            namespace process
            {
               void exit( const common::process::lifetime::Exit& exit);

            } // process

         } // handle

         namespace startup
         {
            handle::dispatch_type handlers( State& state);   
         } // startup

         handle::dispatch_type handlers( State& state);

      } // manager
   } // queue
} // casual


