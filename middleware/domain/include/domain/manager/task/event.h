//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/manager/state.h"
#include "domain/manager/ipc.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace task
         {
            namespace event
            {
               template< typename C> 
               void dispatch( State& state, C&& event_creator)
               {
                  Trace trace{ "domain::manager::task::event::dispatch"};

                  using event_type = std::decay_t< decltype( event_creator())>;

                  if( state.event.active< event_type>() || state.tasks.active( common::message::type( event_type{})))
                  {
                     auto&& event = event_creator();

                     if( state.event.active< event_type>())
                        manager::ipc::send( state, state.event( event));

                     if( state.tasks.active( common::message::type( event)))
                        state.tasks.event( event);
                  }
               }
            } // event
         } // task
      } // manager
   } // domain
} // casual