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
#include "common/file.h"

#include "common/code/raise.h"
#include "common/code/casual.h"


#include "common/communication/instance.h"
#include "common/algorithm.h"

#include <fstream>
#include <iostream>


namespace casual
{
   namespace event::service::log
   {

      namespace local
      {
         namespace
         {
            struct Filter
            {
               std::string inclusive;
               std::string exclusive;

               CASUAL_LOG_SERIALIZE({
                  CASUAL_SERIALIZE( inclusive);
                  CASUAL_SERIALIZE( exclusive);
               })
            };
            struct Settings
            {
               std::string delimiter = "|";
               std::string file = "statistics.log";
               Filter filter;

               CASUAL_LOG_SERIALIZE({
                  CASUAL_SERIALIZE( file);
                  CASUAL_SERIALIZE( delimiter);
                  CASUAL_SERIALIZE( filter);
               })
            };


            namespace state
            {
               struct Log
               {
                  Log( std::string path, std::string delimiter) 
                     : file{ Log::open( path)}, delimiter{ std::move( delimiter)}, path{ std::move( path)} {}
                  
                  std::ofstream file;
                  std::string delimiter;
                  std::string path;

                  void reopen()
                  {  
                     common::log::line( event::log, "reopen: ", path);
                     file.flush();
                     file = open( path);
                  }

                  void flush()
                  {
                     file.flush();
                  }

               private:

                  static std::ofstream open( const std::string& path)
                  {
                     // make sure we got the directory
                     common::directory::create( common::directory::name::base( path));                           

                     std::ofstream file{ path, std::ios::app};

                     if( ! file)
                        common::code::raise::error( common::code::casual::invalid_path, "failed to open file: ", path);

                     return file;
                  }
               };
            } // state

            namespace detail
            {

               template< typename F> 
               void pump( state::Log& log, F&& filter)
               {
                  // make sure we reopen file on SIGHUP
                  common::signal::callback::registration< common::code::signal::hangup>( [&log](){ log.reopen();});

                  bool done = false;

                  auto conditions = common::event::condition::compose(
                     common::event::condition::error( [&done]() 
                     { 
                        auto code = common::exception::code();
                        common::log::line( event::verbose::log, "event listen - condition error: ", code);
                        
                        if( code == common::code::signal::terminate)
                           done = true;
                        else
                           throw;
                     }),
                     common::event::condition::idle( [&log]()
                     {
                        // inbound is idle, 
                        log.flush();
                     }),
                     common::event::condition::done( [&done]() { return done;})
                  );

                  common::event::listen(
                     std::move( conditions),
                     [&log, &filter]( const common::message::event::service::Calls& event)
                     {
                        auto handle = []( auto& log, auto& filter, const common::message::event::service::Metric& metric)
                        {
                           if( ! filter( metric))
                              return;

                           common::log::line( event::verbose::log, "metric: ", metric);

                           log.file << metric.service
                              << log.delimiter << metric.parent
                              << log.delimiter << metric.process.pid
                              << log.delimiter << metric.execution
                              << log.delimiter << metric.trid
                              << log.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.start.time_since_epoch()).count()
                              << log.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.end.time_since_epoch()).count()
                              << log.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.pending).count()
                              << log.delimiter << common::code::description( metric.code)
                              << '\n';
                        };

                        common::algorithm::for_each( event.metrics, [&]( auto& metric)
                        { 
                           handle( log, filter, metric);
                        });
                     }
                  );
               }
            } // detail


            void pump( Settings settings)
            {
               state::Log log{ std::move( settings.file), std::move( settings.delimiter)};

               auto filter_exclusive = [&settings]()
               {
                  return [filter = std::regex{ settings.filter.exclusive}]( auto& metric)
                  {
                     return ! std::regex_match( metric.service, filter);
                  };
               };

               auto filter_inclusive = [&settings]()
               {
                  return [filter = std::regex{ settings.filter.inclusive}]( auto& metric)
                  {
                     return std::regex_match( metric.service, filter);
                  };
               };

               auto filter_combo = [&]()
               {
                  return [exclusive = filter_exclusive(), inclusive = filter_inclusive()]( auto& metric)
                  {
                     return exclusive( metric) && inclusive( metric);
                  };
               };

               if( ! settings.filter.inclusive.empty() && ! settings.filter.exclusive.empty())
                  detail::pump( log, filter_combo());
               else if( ! settings.filter.inclusive.empty())
                  detail::pump( log, filter_inclusive());
               else if( ! settings.filter.exclusive.empty())
                  detail::pump( log, filter_exclusive());
               else
                  detail::pump( log, []( auto& metric){ return true;});
            }


            namespace option
            {
               namespace filter
               {
                  auto exclusive( Settings& settings)
                  {
                     using namespace casual::common::argument;
                     
                     return Option( 
                        std::tie( settings.filter.exclusive), 
                        common::argument::option::keys( { "--filter-exclusive"}, { "--discard"}),
                        "regexp pattern - if matched on service name metrics are discarded");
                  }

                  auto inclusive( Settings& settings)
                  {
                     using namespace casual::common::argument;
                     
                     return Option( 
                        std::tie( settings.filter.inclusive), 
                        common::argument::option::keys( { "--filter-inclusive"}, { "--filter"}),
                        "regexp pattern - only services with matched names will be logged");
                  }
               } // filter
            } // option

            void main( int argc, char **argv)
            {
               Settings settings;

               {
                  using namespace casual::common::argument;
                  Parse{ "service log", 
                     Option( std::tie( settings.file), { "-f", "--file"}, "path to log-file (default: '" + settings.file + "')"),
                     Option( std::tie( settings.delimiter), { "-d", "--delimiter"}, "delimiter between columns (default: '" + settings.delimiter + "')"),
                     option::filter::inclusive( settings),
                     option::filter::exclusive( settings),
                  }( argc, argv);
               }

               common::log::line( event::log, "settings: ", settings);

               // connect to domain
               common::communication::instance::whitelist::connect( 0xc9d132c7249241c8b4085cc399b19714_uuid);

               pump( std::move( settings));
            }        
         } // <unnamed>
      } // local

   } // event::service::log
} // casual



int main( int argc, char **argv)
{
   return casual::common::exception::main::guard( [&]()
   {
      casual::event::service::log::local::main( argc, argv);
   });

}
