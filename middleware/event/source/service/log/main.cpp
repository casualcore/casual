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
                     std::string discard;
                     std::string filter;

                     CASUAL_LOG_SERIALIZE({
                        CASUAL_NAMED_VALUE( file);
                        CASUAL_NAMED_VALUE( delimiter);
                        CASUAL_NAMED_VALUE( discard);
                        CASUAL_NAMED_VALUE( filter);
                     })
                  };

                  template< typename Filter>
                  struct basic_handler
                  {
                     basic_handler( Settings settings) 
                        : m_filter{ settings}, 
                        m_logfile{ basic_handler::open( settings.file)}, 
                        m_file{ std::move( settings.delimiter), 
                        std::move( settings.file)}
                     {
                     }

                     void log( const common::message::event::service::Metric& metric)
                     {
                        if( ! m_filter( metric))
                           return;

                        common::log::line( event::verbose::log, "metric: ", metric);

                        m_logfile << metric.service
                           << m_file.delimiter << metric.parent
                           << m_file.delimiter << metric.process.pid
                           << m_file.delimiter << metric.execution
                           << m_file.delimiter << metric.trid
                           << m_file.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.start.time_since_epoch()).count()
                           << m_file.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.end.time_since_epoch()).count()
                           << m_file.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.pending).count()
                           << m_file.delimiter << common::code::string( metric.code)
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
                        common::log::line( event::log, "reopen: ", m_file.name);
                        m_logfile.flush();
                        m_logfile = open( m_file.name);
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

                     Filter m_filter;

                     std::ofstream m_logfile;
                     struct 
                     {
                        std::string delimiter;
                        std::string name;
                     } m_file;
                  };

                  namespace policy
                  {
                     struct None
                     {
                        None( const Settings&) {}
                        bool operator () ( const common::message::event::service::Metric& metric) const { return true;}

                     };

                     struct Discard
                     {
                        Discard( const Settings& settings) : discard{ settings.discard} {}
                        bool operator () ( const common::message::event::service::Metric& metric) const 
                        {
                            return ! std::regex_match( metric.service, discard);
                        }

                        std::regex discard;
                     };

                     struct Filter
                     {
                        Filter( const Settings& settings) : filter{ settings.filter} {}
                        bool operator () ( const common::message::event::service::Metric& metric) const 
                        {
                            return std::regex_match( metric.service, filter);
                        }

                        std::regex filter;
                     };
                  } // policy

                  namespace detail
                  {
                     template< typename H> 
                     void pump( H&& handler)
                     {
                        // make sure we reopen file on SIGHUP
                        common::signal::callback::registration< common::code::signal::hangup>( [&handler](){ handler.reopen();});

                        common::event::listen(
                           common::event::condition::compose( common::event::condition::idle( [&handler]()
                           {
                              // inbound is idle, 
                              handler.idle();
                           })),
                           [&handler]( common::message::event::service::Calls& event)
                           {
                              handler.log( event);
                           }
                        );
                     }
                  } // detail


                  void pump( Settings settings)
                  {
                     if( ! settings.discard.empty())
                        detail::pump( basic_handler< policy::Discard>{ std::move( settings)});
                     else if( ! settings.filter.empty())
                        detail::pump( basic_handler< policy::Filter>{ std::move( settings)});
                     else
                        detail::pump( basic_handler< policy::None>{ std::move( settings)});
                  }

                  namespace information
                  {
                     constexpr auto discard = "regexp pattern - if matched on service name metrics are discarded\nmutally exclusive with --filter";
                     constexpr auto filter = "regexp pattern - only services with matched names will be logged\nmutally exclusive with --discard";
                  } // information

                  void main( int argc, char **argv)
                  {
                     Settings settings;

                     {
                        using namespace casual::common::argument;
                        Parse{ "service log", 
                           Option( std::tie( settings.file), { "-f", "--file"}, "path to log-file (default: '" + settings.file + "')"),
                           Option( std::tie( settings.delimiter), { "-d", "--delimiter"}, "delimiter between columns (default: '" + settings.delimiter + "')"),
                           Option( std::tie( settings.discard), { "--discard"}, information::discard),
                           Option( std::tie( settings.filter), { "--filter"}, information::filter),
                        }( argc, argv);
                     }

                     common::log::line( event::log, "settings: ", settings);

                     // connect to domain
                     common::communication::instance::connect( 0xc9d132c7249241c8b4085cc399b19714_uuid);

                     pump( std::move( settings));
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
