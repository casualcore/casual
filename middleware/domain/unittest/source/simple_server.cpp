//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/message/handle.h"
#include "common/message/signal.h"
#include "common/exception/handle.h"
#include "common/exception/signal.h"
#include "common/environment.h"

namespace casual
{
   using namespace common;
   namespace local
   {
      namespace
      {
         void main(int argc, char **argv)
         {
            communication::instance::connect();

            auto error_handler = []()
            {
               try
               {
                  throw;
               }
               catch( const exception::signal::Hangup& exception)
               {
                  log::line( log::category::information, "exception: ", exception);

                  // push to our own ipc
                  communication::ipc::inbound::device().push( message::signal::Hangup{});

                  common::environment::variable::set( "CASUAL_SIMPLE_SERVER_HANGUP_SIGNAL", "true");
               }
            };

            communication::ipc::Helper ipc{ std::move( error_handler)};


            auto handle_hangup = []( const message::signal::Hangup& message)
            {
               log::line( log::category::information, "handle_hangup - message ", message);

               common::environment::variable::set( "CASUAL_SIMPLE_SERVER_HANGUP_MESSAGE", "true");
            };



            auto handler = ipc.device().handler(
                  message::handle::Ping{},
                  message::handle::Shutdown{},
                  message::handle::global::State{},
                  std::move( handle_hangup)
            );

            message::dispatch::blocking::pump( handler, ipc);
         }

      } // <unnamed>
   } // local

} // casual

int main(int argc, char **argv)
{
   return casual::common::exception::guard( [&]()
   {
      casual::local::main( argc, argv);
   });
}

