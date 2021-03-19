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

         common::message::pending::Message base_dispatch::pending( common::communication::ipc::message::Complete&& complete) const
         {
            Trace trace{ "common::event::base_dispatch::pending"};

            return common::message::pending::Message{
               std::move( complete),
               m_subscribers};
         }

      } // event
   } // common
} // casual
