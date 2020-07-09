//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/manager.h"

#include "common/exception/handle.h"
#include "common/exception/casual.h"
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
                     try
                     {
                        throw;
                     }
                     catch( const std::exception& exception)
                     {
                        if( event_ipc)
                        {
                           common::message::event::Error event;
                           event.message = exception.what();
                           event.severity = common::message::event::Error::Severity::fatal;

                           common::communication::device::non::blocking::send( event_ipc, event);
                        }
                        return casual::common::exception::handle();
                     }
                     return 666;
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

                  }
                  catch( ...)
                  {
                     return handle::exception( settings.event.ipc);
                  }

                  return 0;
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
