//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/manager.h"

#include "common/exception/handle.h"
#include "common/argument.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {
               namespace handle
               {
                  int exception( common::strong::ipc::id event_ipc)
                  {
                     auto code = common::exception::code();

                     if( event_ipc)
                     {
                        common::message::event::Error event;
                        event.message = common::string::compose( "fatal abort");
                        event.severity = common::message::event::Error::Severity::fatal;
                        event.code = code;

                        common::communication::device::non::blocking::send( event_ipc, event);
                     }
                  
                     return code.value();
                  }
               } // handle

               int main( int argc, char** argv)
               {
                  Settings settings;

                  try
                  {                     
                     common::argument::Parse{ "domain manager",
                        common::argument::Option( std::tie( settings.configurationfiles), { "-c", "--configuration-files"}, "domain configuration files"),
                        common::argument::Option( std::tie( settings.persist), { "--persist"}, "if domain should store current state persistent on shutdown"),
                        common::argument::Option( std::tie( settings.bare), { "--bare"}, "if use 'bare' mode or not, ie, do not boot mandatory (broker, TM), mostly for unittest"),

                        common::argument::Option( std::tie( settings.event.ipc.underlaying()), { "--event-ipc"}, "ipc to send events to"),
                        common::argument::Option( std::tie( settings.event.id), { "--event-id"}, "id of the events to correlate"),
                     }( argc, argv);

                     Manager domain( std::move( settings));
                     domain.start();

                     return 0;
                  }
                  catch( ...)
                  {
                     return handle::exception( settings.event.ipc);
                  }

               }
            } // <unnamed>
         } // local
         

      } // manager
   } // domain
} // casual


int main( int argc, char** argv)
{
   return casual::domain::manager::local::main( argc, argv);
}
