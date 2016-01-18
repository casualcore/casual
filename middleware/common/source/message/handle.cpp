//!
//! handle.cpp
//!
//! Created on: Oct 13, 2014
//!     Author: Lazan
//!

#include "common/message/handle.h"
#include "common/exception.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace handle
         {

            void Shutdown::operator () ( message_type& message)
            {
               throw exception::Shutdown{ "shutdown " + process::path()};
            }

            void Ping::operator () ( server::ping::Request& message)
            {
               log::internal::debug << "pinged by process: " << message.process << '\n';

               server::ping::Reply reply;
               reply.correlation = message.correlation;
               reply.process = process::handle();
               reply.uuid = process::uuid();

               communication::ipc::outbound::Device ipc{ message.process.queue};

               //
               // We ignore signals
               //
               ipc.send( reply, communication::ipc::policy::ignore::signal::Blocking{});
            }

         } // handle
      } // message
   } // common
} // casual
