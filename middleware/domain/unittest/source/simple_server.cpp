//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/message/dispatch/handle.h"
#include "common/message/signal.h"
#include "common/exception/capture.h"
#include "common/environment.h"
#include "common/signal.h"

#include "casual/argument.h"

#include "common/signal.h"

namespace casual
{
   using namespace common;
   namespace local
   {
      namespace
      {
         struct Settings
         {
            bool terminate = false;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( terminate);
            )

         };

         void run( Settings settings)
         {
            if( settings.terminate)
            {
               log::information( "terminating as per settings");
               std::terminate();
            }

            signal::callback::registration< code::signal::hangup>( []()
            {
               log::line( log::category::information, "signal callback - ", code::signal::hangup);

               // push to our own ipc
               communication::ipc::inbound::device().push( message::signal::Hangup{});

               environment::variable::set( "CASUAL_SIMPLE_SERVER_HANGUP_SIGNAL", "true");
            });

            communication::instance::connect();


            auto handle_hangup = []( const message::signal::Hangup& message)
            {
               log::line( log::category::information, "handle_hangup - message ", message);

               environment::variable::set( "CASUAL_SIMPLE_SERVER_HANGUP_MESSAGE", "true");
            };

            auto& ipc = communication::ipc::inbound::device();

            auto handler = message::dispatch::handler( ipc,
                  message::dispatch::handle::defaults(),
                  std::move( handle_hangup)
            );

            message::dispatch::pump( handler, ipc);
         }

         void main(int argc, char **argv)
         {
            Settings settings;
            { 
               casual::argument::parse( "simple server", {
                  casual::argument::Option( argument::option::flag( settings.terminate), {{ "--terminate"}}, "if set, the server will terminate directly" ), 
               }, argc, argv);
            }

            run( std::move( settings));
         }

      } // <unnamed>
   } // local

} // casual

int main(int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [&]()
   {
      casual::local::main( argc, argv);
   });
}

