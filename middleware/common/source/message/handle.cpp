//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/message/handle.h"
#include "common/exception/casual.h"
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
               log::debug << "shutdown received from: " << message.process << '\n';

               throw exception::casual::Shutdown{};
            }

            void Ping::operator () ( server::ping::Request& message)
            {
               log::debug << "pinged by process: " << message.process << '\n';

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
                  signal::thread::scope::Mask mask{ signal::set::filled( signal::Type::terminate, signal::Type::interrupt)};
                  ipc.send( reply, communication::ipc::policy::Blocking{});
               }
               catch( const common::exception::system::communication::Unavailable&)
               {
                  log::debug << "queue unavailable: " << message.process << " - action: ignore\n";
               }
            }

         } // handle
      } // message
   } // common
} // casual
