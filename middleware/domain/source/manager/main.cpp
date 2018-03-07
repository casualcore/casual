//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "domain/manager/manager.h"


#include "common/exception/handle.h"
#include "common/arguments.h"


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
                  common::Arguments parser{ {
                     common::argument::directive( common::argument::cardinality::Any{}, {"-c", "--configuration-files"}, "domain configuration files", settings.configurationfiles),
                     common::argument::directive( { "--no-auto-persist"}, "domain does not store current state persistent on shutdown", settings.no_auto_persist),
                     common::argument::directive( {"--bare"}, "do not boot mandatory (broker, TM), mostly for unittest", settings.bare),


                     common::argument::directive( {"-q", "--event-queue"}, "queue to send events to", settings.event_queue),
                     common::argument::directive( {"-e", "--events"}, "events to send to the queue (process-spawn|process-exit)", settings.event_queue),
                     }};


                  parser.parse( argc, argv);

                  event_queue = common::strong::ipc::id{ settings.event_queue};
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
