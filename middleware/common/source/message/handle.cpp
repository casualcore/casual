//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/message/handle.h"
#include "common/exception/casual.h"
#include "common/communication/ipc.h"
#include "common/environment.h"
#include "common/instance.h"

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
                     try
                     {
                        // We ignore signals
                        signal::thread::scope::Mask mask{ signal::set::filled( code::signal::terminate, code::signal::interrupt)};

                        communication::device::blocking::send( destination.ipc, message);
                     }
                     catch( const common::exception::system::communication::Unavailable&)
                     {
                        log::line( log::debug, "queue unavailable: ",  destination, " - action: ignore");
                     }
                  }
               } // <unnamed>
            } // local

            void Shutdown::operator () ( const message::shutdown::Request& message)
            {
               log::line( log::debug, "shutdown received from: ", message.process);

               throw exception::casual::Shutdown{};
            }

            void Ping::operator () ( const server::ping::Request& message)
            {
               log::line( log::debug, "pinged by process: ", message.process);

               auto reply = message::reverse::type( message);
               reply.process = common::process::handle();

               local::send( message.process, reply);
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
