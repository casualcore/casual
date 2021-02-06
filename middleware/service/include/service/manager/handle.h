//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "service/manager/state.h"

#include "common/message/dispatch.h"
#include "common/message/gateway.h"
#include "common/message/transaction.h"
#include "common/message/event.h"

#include "common/server/handle/call.h"
#include "common/server/context.h"


namespace casual
{
   namespace service::manager
   {
      namespace ipc
      {
         common::communication::ipc::inbound::Device& device();
      } // ipc

      namespace handle
      {
         namespace comply
         {
            void configuration( State& state, casual::configuration::Model model);
         } // comply

         void timeout( State& state);

         namespace metric
         {
            //! tries to send metrics regardless
            void send( State& state);

            namespace batch
            {
               //! send metrics if we've reach batch-limit
               void send( State& state);
            } // batch
            
         } // metric

         namespace process
         {
            void exit( const common::process::lifetime::Exit& exit);   
         } // process

         using dispatch_type = decltype( common::message::dispatch::handler( ipc::device()));

      } // handle
      
      //! @returns all the handlers for service manager
      handle::dispatch_type handler( State& state);

   } // service::manager
} // casual



