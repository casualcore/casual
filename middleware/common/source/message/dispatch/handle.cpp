//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/message/dispatch/handle.h"
#include "common/message/counter.h"
#include "common/communication/ipc.h"
#include "common/environment.h"
#include "common/instance.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/log/stream.h"

namespace casual
{
   namespace common::message::dispatch::handle
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

            namespace handle
            {
               auto ping()
               {
                  return []( const server::ping::Request& message)
                  {
                     Trace trace{ "common::message::dispatch::handle::local::handle::ping"};
                     log::line( log::debug, "message: ", message);

                     local::send( message.process, message::reverse::type( message, common::process::handle()));
                  };
               }

               auto shutdown()
               {
                  return []( const message::shutdown::Request& message)
                  {
                     Trace trace{ "common::message::dispatch::handle::local::handle::shutdown"};
                     log::line( log::debug, "message: ", message);

                     code::raise::error( code::casual::shutdown, "shutdown received from: ", message.process);
                  };
               }

               namespace global
               {
                  auto state()
                  {
                     return []( const message::domain::instance::global::state::Request& message)
                     {
                        Trace trace{ "common::message::dispatch::handle::local::handle::global::state"};
                        log::line( log::debug, "message: ", message);

                        auto reply = message::reverse::type( message);
                        reply.process.handle = common::process::handle();
                        reply.process.path = common::process::path();
                        reply.environment.variables = environment::variable::current();
                        if( auto& instance = instance::information())
                        {
                           reply.instance.alias = instance.value().alias;
                           reply.instance.index = instance.value().index;
                        }

                        log::line( log::debug, "reply: ", reply);
                        
                        local::send( message.process, reply);
                     };
                  }
               } // global

               namespace configure
               {
                  auto log()
                  {
                     return []( message::internal::configure::Log& message)
                     {
                        Trace trace{ "common::message::dispatch::handle::local::handle::configure::log"};
                        log::line( log::debug, "message: ", message);

                        log::stream::Configure configure;
                        configure.path = message.path;
                        configure.expression.inclusive = message.expression.inclusive;

                        log::stream::configure( configure);
                     };
                  }
               } // configure

               auto counter()
               {
                  return []( const message::counter::Request& message)
                  {
                     Trace trace{ "common::message::dispatch::handle::local::handle::counter"};
                     log::line( log::debug, "message: ", message);

                     auto reply = message::reverse::type( message);
                     reply.entries = message::counter::entries();

                     log::line( log::debug, "reply: ", reply);
                     local::send( message.process, reply);
                  };
               }

            } // handle
         } // <unnamed>
      } // local


      handler_type defaults() noexcept
      {
         return dispatch::handler( communication::ipc::inbound::device(),
            local::handle::ping(),
            local::handle::shutdown(),
            local::handle::configure::log(),
            local::handle::global::state(), 
            local::handle::counter());
      }

   } // common::message::dispatch::handle
} // casual
