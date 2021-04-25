//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/message/handle.h"
#include "common/communication/ipc.h"
#include "common/environment.h"
#include "common/instance.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace handle
         {
            namespace local
            {
               namespace
               {
                  template< typename M>
                  void send( const common::process::Handle& destination, M&& message)
                  {

                     // We ignore signals
                     signal::thread::scope::Mask mask{ signal::set::filled( code::signal::terminate, code::signal::interrupt)};

                     communication::device::blocking::optional::send( destination.ipc, message);
                  }
               } // <unnamed>
            } // local

            void Shutdown::operator () ( const message::shutdown::Request& message)
            {
               code::raise::error( code::casual::shutdown, "shutdown received from: ", message.process);
            }

            void Ping::operator () ( const server::ping::Request& message)
            {
               log::line( log::debug, "pinged by process: ", message.process);
               local::send( message.process, message::reverse::type( message, common::process::handle()));
            }

            
            namespace global
            {
               void State::operator () ( const message::domain::instance::global::state::Request& message)
               {
                  Trace trace{ "common::message::handle::global::State"};
                  log::line( log::debug, "message: ", message);

                  auto reply = message::reverse::type( message);
                  reply.process.handle = common::process::handle();
                  reply.process.path = common::process::path();
                  reply.environment.variables = environment::variable::native::current();
                  if( auto& instance = instance::information())
                  {
                     reply.instance.alias = instance.value().alias;
                     reply.instance.index = instance.value().index;
                  }

                  log::line( log::debug, "reply: ", reply);
                  
                  local::send( message.process, reply);
               }
            } // global

         } // handle
      } // message
   } // common
} // casual
