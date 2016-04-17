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
               log::internal::debug << "shutdown received from: " << message.process << '\n';

               throw exception::Shutdown{ "shutdown " + common::process::path()};
            }

            void Ping::operator () ( server::ping::Request& message)
            {
               log::internal::debug << "pinged by process: " << message.process << '\n';

               server::ping::Reply reply;
               reply.correlation = message.correlation;
               reply.process = common::process::handle();
               reply.uuid = common::process::uuid();

               communication::ipc::outbound::Device ipc{ message.process.queue};

               //
               // We ignore signals
               //
               try
               {
                  signal::thread::scope::Mask mask{ signal::set::filled( { signal::Type::terminate, signal::Type::terminate})};
                  ipc.send( reply, communication::ipc::policy::Blocking{});
               }
               catch( common::exception::queue::Unavailable&)
               {
                  log::internal::debug << "queue unavailable: " << message.process << " - action: ignore\n";
               }
            }

         } // handle
      } // message
   } // common
} // casual
