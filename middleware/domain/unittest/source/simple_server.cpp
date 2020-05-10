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
#include "common/signal.h"

namespace casual
{
   using namespace common;
   namespace local
   {
      namespace
      {
         void main(int argc, char **argv)
         {
            
            signal::callback::registration< code::signal::hangup>( []()
            {
               log::line( log::category::information, "signal callback - ", code::signal::hangup);

               // push to our own ipc
               communication::ipc::inbound::device().push( message::signal::Hangup{});

               common::environment::variable::set( "CASUAL_SIMPLE_SERVER_HANGUP_SIGNAL", "true");
            });

            communication::instance::connect();


            auto handle_hangup = []( const message::signal::Hangup& message)
            {
               log::line( log::category::information, "handle_hangup - message ", message);

               common::environment::variable::set( "CASUAL_SIMPLE_SERVER_HANGUP_MESSAGE", "true");
            };

            auto& ipc = communication::ipc::inbound::device();

            auto handler = ipc.handler(
                  message::handle::defaults( ipc),
                  std::move( handle_hangup)
            );

            message::dispatch::pump( handler, ipc);
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

