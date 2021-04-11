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
   namespace domain::manager::handle
   {
      using dispatch_type = decltype( common::message::dispatch::handler( ipc::device()));   

      namespace mandatory
      {
         namespace boot
         {
            namespace core
            {
               void prepare( State& state);
            } // core

            void prepare( State& state);
         } // boot

      } // mandatory

      void boot( State& state, common::strong::correlation::id correlation);

      std::vector< common::strong::correlation::id> shutdown( State& state);

      namespace start
      {
         namespace pending
         {
            //void message( State& state);
         } // pending
      } // start

      namespace scale
      {
         void shutdown( State& state, std::vector< common::process::Handle> processes);

         void instances( State& state, state::Server& server);
         void instances( State& state, state::Executable& executable);

         std::vector< common::strong::correlation::id> aliases( State& state, std::vector< admin::model::scale::Alias> aliases);

      } // scale

   
      namespace restart
      {
         std::vector< common::strong::correlation::id> aliases( State& state, std::vector< std::string> aliases);
         std::vector< common::strong::correlation::id> groups( State& state, std::vector< std::string> groups);
      } // restart

      namespace event
      {
         namespace process
         {
            void exit( const common::process::lifetime::Exit& exit);
         } // process

      } // event


      handle::dispatch_type create( State& state);

   } // domain::manager::handle
} // casual


