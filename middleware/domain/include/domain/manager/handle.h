//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/state.h"
#include "domain/manager/ipc.h"
#include "domain/manager/task.h"
#include "domain/manager/admin/model.h"

#include "common/message/type.h"
#include "common/message/domain.h"
#include "common/message/dispatch.h"


namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace handle
         {
            using dispatch_type = decltype( ipc::device().handler());

            namespace mandatory
            {
               namespace boot
               {
                  void prepare( State& state);
               } // boot

            } // mandatory

            void boot( State& state, common::Uuid correlation);

            std::vector< common::Uuid> shutdown( State& state);

            namespace start
            {
               namespace pending
               {
                  common::Process message();
               } // pending
            } // start

            namespace scale
            {
               void shutdown( State& state, std::vector< common::process::Handle> processes);

               void instances( State& state, state::Server& server);
               void instances( State& state, state::Executable& executable);

               std::vector< common::Uuid> aliases( State& state, std::vector< admin::model::scale::Alias> aliases);

            } // scale

         
            namespace restart
            {
               std::vector< common::Uuid> instances( State& state, std::vector< std::string> aliases);
            } // restart

            namespace event
            {
               namespace process
               {
                  void exit( const common::process::lifetime::Exit& exit);
               } // process

            } // event

         } // handle

         handle::dispatch_type handler( State& state);

      } // manager
   } // domain
} // casual


