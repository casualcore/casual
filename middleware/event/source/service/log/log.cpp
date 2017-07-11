//!
//! casual
//!


#include "common/arguments.h"
#include "common/process.h"
#include "common/uuid.h"
#include "common/event/listen.h"

#include <fstream>
#include <iostream>


namespace casual
{
   namespace event
   {
      namespace log
      {


         struct Handler
         {
            Handler( const std::string& file) : m_logfile{ file}
            {

            }

            void log( const common::message::event::service::Call& event)
            {
               m_logfile << event.service
                     << "|" << event.parent
                     << "|" << event.process.pid
                     << "|" << event.execution
                     << "|" << event.trid
                     << "|" << std::chrono::duration_cast< std::chrono::microseconds>( event.start.time_since_epoch()).count()
                     << "|" << std::chrono::duration_cast< std::chrono::microseconds>( event.end.time_since_epoch()).count()
                     << '\n';
            }

            void idle()
            {
               m_logfile.flush();
            }

         private:
            std::ofstream m_logfile;
         };


         void main(int argc, char **argv)
         {
            // get log-file from arguments
            std::string file{"statistics.log"};
            {
               casual::common::Arguments parser{
                  { casual::common::argument::directive( { "-f", "--file"}, "path to log-file", file)}
               };

               parser.parse( argc, argv);
            }

            //
            // connect to domain
            //
            common::process::instance::connect( common::Uuid{ "c9d132c7249241c8b4085cc399b19714"});

            {
               Handler handler{ file};

               common::event::idle::listen(
                     [&](){
                     //
                     // the queue is empty
                     //
                     handler.idle();
                  },
                  [&]( common::message::event::service::Call& event){
                     handler.log( event);
                  });
            }
         }
      } // log
   } // event
} // casual



int main( int argc, char **argv)
{
   try
   {
      casual::event::log::main( argc, argv);
      return 0;
   }
   catch( ...)
   {
      return casual::common::error::handler();
   }

}
