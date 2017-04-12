//!
//! casual 
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
               range::transform( subscribers, std::mem_fn( &common::process::Handle::queue))
            };
         }

         void base_dispatch::remove( platform::pid::type pid)
         {
            Trace trace{ "common::event::base_dispatch::remove"};

            auto split = range::partition( subscribers, [pid]( const auto& p){
               return p.pid != pid;
            });

            range::erase( subscribers, std::get< 1>( split));
         }

         bool base_dispatch::exists( platform::ipc::id::type queue) const
         {
            return range::find_if( subscribers, [queue]( auto& s){
               return s.queue == queue;
            });
         }


      } // event
   } // common
} // casual
