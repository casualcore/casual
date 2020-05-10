//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "queue/group/group.h"

#include "common/message/queue.h"
#include "common/message/signal.h"
#include "common/message/dispatch.h"
#include "common/message/transaction.h"
#include "common/message/event.h"

namespace casual
{

   namespace queue
   {
      namespace group
      {
         namespace handle
         {

            namespace ipc
            {
               common::communication::ipc::inbound::Device& device();
            }

            using dispatch_type = common::communication::ipc::dispatch::Handler;

            void shutdown( State& state);


            //! * persist the queuebase
            //! * send pending replies, if any
            //! * check if pending request has some messages to consume
            void persist( State& state);


         } // handle

         handle::dispatch_type handler( State& state);

      } // group
   } // queue


} // casual


