//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "queue/common/ipc.h"
#include "queue/group/state.h"

#include "common/message/dispatch.h"


namespace casual
{
   namespace queue::group
   {
      namespace handle
      {      
         using dispatch_type = decltype( common::message::dispatch::handler( ipc::device()));

         void shutdown( State& state);

         //! hard shutdown - best effort shutdown
         void abort( State& state);

         //! * persist the queuebase
         //! * send pending replies, if any
         //! * check if pending request has some messages to consume
         void persist( State& state);

      } // handle

      handle::dispatch_type handlers( State& state);

   } // queue::group

} // casual


