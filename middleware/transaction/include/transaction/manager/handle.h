//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "transaction/manager/state.h"

#include "common/message/dispatch.h"
#include "common/communication/ipc.h"


namespace casual
{
   namespace transaction::manager
   {
      namespace ipc
      {
         common::communication::ipc::inbound::Device& device();
      } // ipc


      namespace handle
      {
         namespace persist
         {
            //! persist and send pending replies, if any
            void send( State& state);
         } // persist

         

         namespace process
         {
            void exit( const common::process::lifetime::Exit& exit);

         } // process

         using dispatch_type = decltype( common::message::dispatch::handler( ipc::device()));

         namespace startup
         {
            //! return the handlers used during startup
            dispatch_type handlers( State& state);
         } // startup

         dispatch_type handlers( State& state);

         void abort( State& state);
         

      } // handle

   } // transaction::manager
} // casual



