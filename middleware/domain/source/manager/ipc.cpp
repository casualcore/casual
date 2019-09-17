//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/ipc.h"

#include "domain/manager/state.h"
#include "domain/manager/handle.h"
#include "domain/pending/message/message.h"

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {
         struct State;

         namespace ipc
         {
            const communication::ipc::Helper& device()
            {
               static communication::ipc::Helper singleton{
                  communication::error::handler::callback::on::Terminate{ &handle::event::process::exit}};

               return singleton;
            }

            namespace pending
            {
               void send( const State& state, common::message::pending::Message&& pending)
               {
                  ipc::device().blocking_send( 
                     state.process.pending.handle().ipc, 
                     casual::domain::pending::message::Request{ std::move( pending)});
               }
               
            } // pending

            void send( const State& state, common::message::pending::Message&& pending)
            {
                if( ! message::pending::non::blocking::send( pending, manager::ipc::device().error_handler()))
                     ipc::pending::send( state, std::move( pending));
            }

         } // ipc
      } // manager
   } // domain
} // casual