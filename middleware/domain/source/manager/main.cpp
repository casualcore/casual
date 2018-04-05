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

         int main( int argc, char** argv)
         {

            common::strong::ipc::id event_queue;

            try
            {

               Settings settings;

               {
                  common::argument::Parse parse{ "domain manager",
                     common::argument::Option( std::tie( settings.configurationfiles), { "-c", "--configuration-files"}, "domain configuration files"),
                     common::argument::Option( [&](){ settings.no_auto_persist = true;}, { "--no-auto-persist"}, "domain does not store current state persistent on shutdown"),
                     common::argument::Option( [&](){ settings.bare = true;}, { "--bare"}, "do not boot mandatory (broker, TM), mostly for unittest"),

                     common::argument::Option( [&]( common::strong::ipc::id::value_type v){ settings.event( v);}, { "-q", "--event-queue"}, "queue to send events to"),
                     common::argument::Option( std::tie( settings.events), { "-e", "--events"}, "events to send to the queue (process-spawn|process-exit)"),
                     };


                  parse( argc, argv);
                  event_queue = settings.event();
               }

               Manager domain( std::move( settings));
               domain.start();

            }
            catch( const common::exception::base& exception)
            {
               if( event_queue)
               {
                  common::message::event::domain::Error event;
                  event.message = exception.what();
                  event.severity = common::message::event::domain::Error::Severity::fatal;

                  common::communication::ipc::non::blocking::send( event_queue, event);
               }
               return casual::common::exception::handle();
            }
            catch( ...)
            {
               return casual::common::exception::handle();

            }
            return 0;
         }

      } // manager
   } // domain
} // casual


int main( int argc, char** argv)
{
   return casual::domain::manager::main( argc, argv);
}
