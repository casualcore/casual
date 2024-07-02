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

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/file.h"

#include "common/communication/instance.h"
#include "common/algorithm.h"

#include <iostream>
#include <filesystem>


namespace casual
{
   namespace event::service::log
   {

      namespace local
      {
         namespace
         {
            namespace settings
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
            } // settings

            struct Settings
            {
               std::filesystem::path file = "statistics.log";
               std::string delimiter = "|";
               settings::Filter filter;

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
                  Log( std::filesystem::path path, std::string delimiter) 
                     : file{ std::move( path)}, delimiter{ std::move( delimiter)} {}
                  
                  common::file::Output file;
                  std::string delimiter;

                  void reopen()
                  {  
                     common::log::line( event::log, "reopen service event log: ", file);
                     file = std::move( file).reopen();

                     if( ! file)
                        common::log::line( common::log::category::error ? common::log::category::error : std::cerr, common::code::casual::invalid_file, " failed to reopen event log: ", file);
                  }

                  void flush()
                  {
                     file.flush();
                  }
               };
            } // state

            namespace detail
            {

               template< typename F> 
               void pump( state::Log& log, F filter)
               {
                  bool done = false;

                  // make sure we reopen on SIGHUP
                  common::signal::callback::registration< common::code::signal::hangup>( [ &log]()
                  {
                     // we've highjacked the hangup signal handler for _log rotate_, we
                     // need to call this explicitly
                     common::log::stream::reopen();
                     log.reopen();
                  });


                  auto conditions = common::event::condition::compose(
                     common::event::condition::prelude( []()
                     {
                        // connect to domain
                        common::communication::instance::whitelist::connect( 0xc9d132c7249241c8b4085cc399b19714_uuid);
                     }),
                     common::event::condition::error( [&done]( auto& error) 
                     {
                        common::log::line( event::verbose::log, "event listen - condition error: ", error);
                        
                        if( error.code() == common::code::signal::terminate)
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
                     [ &log, filter = std::move( filter)]( const common::message::event::service::Calls& event)
                     {
                        auto handle = []( auto& log, auto& filter, const common::message::event::service::Metric& metric)
                        {
                           if( ! filter( metric))
                              return;
                           
                           common::log::line( event::verbose::log, "metric: ", metric);

                           auto service_category = []( auto type){ return type == decltype( type)::sequential ? 'S' : 'C';};

                           // if the file is "broken", we try to reopen it. Mostly to notify user
                           if( ! log.file)
                              log.reopen();

                           log.file << metric.service
                              << log.delimiter << metric.parent.service
                              << log.delimiter << metric.process.pid
                              << log.delimiter << metric.execution
                              << log.delimiter << metric.trid
                              << log.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.start.time_since_epoch()).count()
                              << log.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.end.time_since_epoch()).count()
                              << log.delimiter << std::chrono::duration_cast< std::chrono::microseconds>( metric.pending).count()
                              << log.delimiter << common::code::description( metric.code)
                              << log.delimiter << service_category( metric.type)
                              << log.delimiter << metric.span
                              << log.delimiter << metric.parent.span
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
               common::log::line( event::log, "settings: ", settings);

               state::Log log{ std::move( settings.file), std::move( settings.delimiter)};

               auto filter_exclusive = []( auto& settings)
               {
                  return [ filter = std::regex{ settings.filter.exclusive}]( auto& metric) noexcept
                  {
                     return ! std::regex_match( metric.service, filter);
                  };
               };

               auto filter_inclusive = []( auto& settings)
               {
                  return [ filter = std::regex{ settings.filter.inclusive}]( auto& metric) noexcept
                  {
                     return std::regex_match( metric.service, filter);
                  };
               };

               if( ! settings.filter.inclusive.empty() && ! settings.filter.exclusive.empty())
                  detail::pump( log, common::predicate::conjunction( filter_exclusive( settings), filter_inclusive( settings)));
               else if( ! settings.filter.inclusive.empty())
                  detail::pump( log, filter_inclusive( settings));
               else if( ! settings.filter.exclusive.empty())
                  detail::pump( log, filter_exclusive( settings));
               else
                  detail::pump( log, []( auto& metric) noexcept { return true;});
            }


            namespace option
            {
               namespace filter
               {
                  auto completer = []( auto&&, auto help) -> std::vector< std::string>
                  {
                     if( help)
                        return { "<regex>"};
                     return { "<value>"};
                  };

                  auto exclusive( Settings& settings)
                  {                     
                     return casual::common::argument::Option( 
                        std::tie( settings.filter.exclusive), 
                        completer,
                        common::argument::option::keys( { "--filter-exclusive"}, { "--discard"}),
                        "only services that does NOT match the expression are logged\n\n"
                        "can be used in conjunction with --filter-inclusive");
                  }

                  auto inclusive( Settings& settings)
                  {                     
                     return common::argument::Option( 
                        std::tie( settings.filter.inclusive),
                        completer,
                        common::argument::option::keys( { "--filter-inclusive"}, { "--filter"}),
                        "only services that matches the expression are logged\n\n"
                        "can be used in conjunction with --filter-exclusive");
                  }
               } // filter

               auto file( Settings& settings)
               {
                  auto completer = []( auto&&, auto help) -> std::vector< std::string>
                  {
                     if( help)
                        return { "<path>"};
                     return { "<value>"};
                  };
                  return common::argument::Option( 
                     std::tie( settings.file),
                     completer,
                     { "-f", "--file"},
                     common::string::compose( "where to log (default: ", settings.file, ")")); 
               }

               auto delimiter( Settings& settings)
               {
                  auto completer = []( auto&&, auto help) -> std::vector< std::string>
                  {
                     if( help)
                        return { "<string>"};
                     return { "<value>"};
                  };
                  return common::argument::Option( 
                     std::tie( settings.file),
                     completer,
                     { "-d", "--delimiter"},
                     common::string::compose( "delimiter between columns (default: '", settings.delimiter , "')")); 
               }

            } // option

            void main( int argc, char **argv)
            {
               Settings settings;

               {
                  using namespace casual::common::argument;
                  Parse{ "log service call metrics", 
                     option::file( settings),
                     option::delimiter( settings),
                     option::filter::inclusive( settings),
                     option::filter::exclusive( settings),
                  }( argc, argv);
               }

               pump( std::move( settings));
            }        
         } // <unnamed>
      } // local

   } // event::service::log
} // casual



int main( int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [&]()
   {
      casual::event::service::log::local::main( argc, argv);
   });

}
