//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "queue/manager/state.h"
#include "queue/common/ipc.h"

#include "common/message/dispatch.h"

namespace casual
{
   namespace queue::manager::handle
   {

      using dispatch_type = decltype( common::message::dispatch::handler( ipc::device()));

      namespace process
      {
         void exit( State& state, const common::process::lifetime::Exit& exit);

      } // process

      namespace comply
      {
         void configuration( State& state, casual::configuration::Model model);
         
      } // comply

      void idle( State& state);

      //! hard shutdown - best effort shutdown
      void abort( State& state);

      handle::dispatch_type create( State& state);

   } // queue::manager::handle
} // casual


