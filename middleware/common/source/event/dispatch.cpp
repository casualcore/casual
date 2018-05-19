//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/event/dispatch.h"

#include "common/communication/ipc.h"


namespace casual
{
   namespace common
   {
      namespace event
      {

         common::message::pending::Message base_dispatch::pending( common::communication::message::Complete&& complete) const
         {
            Trace trace{ "common::event::base_dispatch::post"};

            return common::message::pending::Message{
               std::move( complete),
               algorithm::transform( m_subscribers, std::mem_fn( &common::process::Handle::queue))
            };
         }

         void base_dispatch::remove( strong::process::id pid)
         {
            Trace trace{ "common::event::base_dispatch::remove"};

            auto split = algorithm::partition( m_subscribers, [pid]( const auto& p){
               return p.pid != pid;
            });

            algorithm::erase( m_subscribers, std::get< 1>( split));
         }

         bool base_dispatch::exists( strong::ipc::id queue) const
         {
            return ! algorithm::find_if( m_subscribers, [queue]( auto& s){
               return s.queue == queue;
            }).empty();
         }


      } // event
   } // common
} // casual
