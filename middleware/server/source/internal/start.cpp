//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "server/internal/start.h"
#include "server/context.h"

#include "common/log.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/message/dispatch.h"
#include "common/message/dispatch/handle.h"

namespace casual
{
   namespace server::internal
   {
      void start( server::Arguments arguments, common::function<void()const> initialize)
      {
         common::Trace trace{ "server::internal::start"};
         common::log::debug( "arguments: ", arguments);


         auto& inbound = common::communication::ipc::inbound::device();

         struct 
         {
            bool done = false;
         } state;

         auto handler = common::message::dispatch::handler( inbound,
            common::message::dispatch::handle::defaults(),
            server::context().initialize( std::move( arguments)),
            [&state]( const common::message::shutdown::Request& message)
            {
               common::log::debug( "shutdown: ", message);
               state.done = true;
            });

         auto condition = common::message::dispatch::condition::compose( 
            common::message::dispatch::condition::prelude(
               [initialize = std::move( initialize)]()
               {
                  common::Trace trace{ "server::internal::start::prelude"};

                  if( initialize)
                     initialize();

                  // Connect to domain - send "ready"...
                  common::communication::instance::connect();
               }),
            common::message::dispatch::condition::done(
                  [&state]() { return state.done;}
               )
         );

         // Start the message-pump
         common::message::dispatch::pump(
            std::move( condition),
            handler,
            inbound);
      }

   } // server::internal
   
} // casual
