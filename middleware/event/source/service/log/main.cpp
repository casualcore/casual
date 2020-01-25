//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/event/common.h"

#include "common/argument.h"
#include "common/process.h"
#include "common/uuid.h"
#include "common/event/listen.h"
#include "common/exception/handle.h"
#include "common/exception/system.h"
#include "common/exception/signal.h"
#include "common/communication/instance.h"
#include "common/algorithm.h"

#include <fstream>
#include <iostream>


namespace casual
{
   namespace event
   {
      namespace service
      {
         namespace log
         {
            namespace local
            {
               namespace
               {
                  struct Settings
                  {
                     std::string delimiter = "|";
                     std::string file = "statistics.log";

                     CASUAL_CONST_CORRECT_SERIALIZE_WRITE({
                        CASUAL_NAMED_VALUE( file);
                        CASUAL_NAMED_VALUE( delimiter);
                     })
                  };

                  struct Handler
                  {
                     Handler( Settings settings) 
                        : m_logfile{ Handler::open( settings.file)}, m_settings{ std::move( settings)}
                     {
                        common::log::line( event::log, "settings: ", m_settings);
                     }

                     void log( const common::message::event::service::Metric& metric)
                     {
                        common::log::line( event::verbose::log, "metric: ", metric);

                        m_logfile << metric.service
                           << m_settings.delimiter << metric.parent
                           << m_settings.delimiter << metric.process.pid
                           << m_settings.delimiter << metric.execution
                           << m_settings.delimiter << metric.trid
                           << m_settings.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.start.time_since_epoch()).count()
                           << m_settings.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.end.time_since_epoch()).count()
                           << m_settings.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.pending).count()
                           << m_settings.delimiter << common::code::string( metric.code)
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

                     void reopen()
                     {
                        common::log::line( event::log, "reopen: ",  m_settings.file);
                        m_logfile.flush();
                        m_logfile = open( m_settings.file);
                     }

                  private:
                     static std::ofstream open( const std::string& name)
                     {
                        // make sure we got the directory
                        common::directory::create( common::directory::name::base( name));                           

                        std::ofstream file{ name, std::ios::app};

                        if( ! file)
                           throw common::exception::system::invalid::Argument{ "failed to open file: " + name};

                        return file;
                     }

                     std::ofstream m_logfile;
                     Settings m_settings;
                  };

                  void main(int argc, char **argv)
                  {
                     Settings settings;

                     {
                        using namespace casual::common::argument;
                        Parse parse{ "service log", 
                           Option( std::tie( settings.file), { "-f", "--file"}, "path to log-file (default: '" + settings.file + "')"),
                           Option( std::tie( settings.delimiter), { "-d", "--delimiter"}, "delimiter between columns (default: '" + settings.delimiter + "')"),
                        };
                        parse( argc, argv);
                     }

                     // connect to domain
                     common::communication::instance::connect( 0xc9d132c7249241c8b4085cc399b19714_uuid);

                     Handler handler{ std::move( settings)};

                     // make sure we reopen file on SIGHUP
                     common::signal::callback::registration< common::code::signal::hangup>( [&handler](){ handler.reopen();});

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

                           
               } // <unnamed>
            } // local
         } // log
      } // service
   } // event
} // casual



int main( int argc, char **argv)
{
   return casual::common::exception::guard( [&]()
   {
      casual::event::service::log::local::main( argc, argv);
   });

}
