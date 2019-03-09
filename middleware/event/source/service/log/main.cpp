//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/argument.h"
#include "common/process.h"
#include "common/uuid.h"
#include "common/event/listen.h"
#include "common/exception/handle.h"
#include "common/communication/instance.h"
#include "common/algorithm.h"

#include <fstream>
#include <iostream>


namespace casual
{
   namespace event
   {
      namespace log
      {
         namespace local
         {
            namespace
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

                  void log( const common::message::event::service::Metric& metric)
                  {
                     m_logfile << metric.service
                        << m_delimiter << metric.parent
                        << m_delimiter << metric.process.pid
                        << m_delimiter << metric.execution
                        << m_delimiter << metric.trid
                        << m_delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.start.time_since_epoch()).count()
                        << m_delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.end.time_since_epoch()).count()
                        << m_delimiter << common::code::string( metric.code)
                        << '\n';
                  }

                  void log( const common::message::event::service::Calls& event)
                  {
                     common::algorithm::for_each( event.metrics, [&]( auto& metric){ log( metric);});
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
                     using namespace casual::common::argument;
                     Parse parse{ "service log", 
                        Option( std::tie( settings.file), { "-f", "--file"}, "path to log-file (default: '" + settings.file + "'"),
                        Option( std::tie( settings.delimiter), { "-d", "--delimiter"}, "delimiter between columns (default: '" + settings.delimiter + "'"),
                     };
                     parse( argc, argv);
                  }

                  // connect to domain
                  common::communication::instance::connect( common::Uuid{ "c9d132c7249241c8b4085cc399b19714"});

                  {
                     Handler handler{ std::move( settings)};

                     common::event::idle::listen(
                        [&handler]()
                        {
                           // the queue is empty
                           handler.idle();
                        },
                        [&handler]( common::message::event::service::Calls& event)
                        {
                           handler.log( event);
                        }
                     );
                  }
               }

                         
            } // <unnamed>
         } // local
      } // log
   } // event
} // casual



int main( int argc, char **argv)
{
   try
   {
      casual::event::log::local::main( argc, argv);
      return 0;
   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }

}
