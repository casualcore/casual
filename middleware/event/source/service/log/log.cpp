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

         struct Settings
         {
            std::string file = "statistics.log";
            std::string delimiter = "|";
         };

         struct Handler
         {
            Handler( Settings settings) 
               : m_logfile{ settings.file, std::ios::app}, m_delimiter{ std::move( settings.delimiter)}
            {

            }

            void log( const common::message::event::service::Call& event)
            {
               m_logfile << event.service
                     << m_delimiter << event.parent
                     << m_delimiter << event.process.pid
                     << m_delimiter << event.execution
                     << m_delimiter << event.trid
                     << m_delimiter << std::chrono::duration_cast< std::chrono::microseconds>( event.start.time_since_epoch()).count()
                     << m_delimiter << std::chrono::duration_cast< std::chrono::microseconds>( event.end.time_since_epoch()).count()
                     << '\n';
            }

            void idle()
            {
               m_logfile.flush();
            }

         private:
            std::ofstream m_logfile;
            std::string m_delimiter;
         };


         void main(int argc, char **argv)
         {
            Settings settings;

            {
               casual::common::Arguments parser{{ 
                  casual::common::argument::directive( { "-f", "--file"}, "path to log-file (default: '" + settings.file + "'", settings.file),
                  casual::common::argument::directive( { "-d", "--delimiter"}, "delimiter between columns (default: '" + settings.delimiter + "'" , settings.delimiter),
               }};

               parser.parse( argc, argv);
            }

            //
            // connect to domain
            //
            common::process::instance::connect( common::Uuid{ "c9d132c7249241c8b4085cc399b19714"});

            {
               Handler handler{ std::move( settings)};

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
      return casual::common::exception::handle();
   }

}
