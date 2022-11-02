//! 
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/administration/cli.h"

#include "common/algorithm.h"
#include "common/terminal.h"
#include "common/strong/id.h"
#include "common/message/internal.h"
#include "common/communication/instance.h"
#include "common/build.h"

#include "casual/cli/message.h"
#include "casual/cli/pipe.h"

#include "domain/manager/admin/cli.h"
#include "domain/discovery/admin/cli.h"
#include "service/manager/admin/cli.h"
#include "queue/manager/admin/cli.h"
#include "transaction/manager/admin/cli.h"
#include "gateway/manager/admin/cli.h"
#include "casual/buffer/admin/cli.h"
#include "tools/service/call/cli.h"
#include "tools/service/describe/cli.h"
#include "configuration/admin/cli.h"

namespace casual
{
   using namespace common;

   namespace administration
   {
      namespace local
      {
         namespace
         {
            namespace information
            {

               using namespace std::string_view_literals;
               constexpr auto names() noexcept { return array::make( 
                  "information-domain"sv, 
                  "information-service"sv, 
                  "information-queue"sv, 
                  "information-transaction"sv);}

               template< typename CLI>
               auto option( CLI& cli)
               {
                  auto complete = []( auto values, bool help) -> std::vector< std::string>
                  {
                     if( help)
                        return { "<value>"};
                     return algorithm::container::create< std::vector< std::string>>( information::names());
                  };

                  auto invoke = [ &cli, complete]( std::vector< std::string> managers)
                  {
                     stream::write( std::cout, "managers: ", managers, '\n');

                     // if not provided we collect from all
                     if( managers.empty())
                        managers = complete( 0, false);

                     stream::write( std::cout, "managers: ", managers, '\n');

                     using information_t = decltype( cli.domain.information());

                     auto append_information = []( auto& cli)
                     {
                        return [&cli]( auto& information)
                        {
                           algorithm::append( cli.information(), information);
                        };
                     };

                     const std::vector< std::tuple< std::string_view, common::function< void( information_t&) const>>> mapping{
                        { "information-domain", append_information( cli.domain)},
                        { "information-service", append_information( cli.service)},
                        { "information-queue", append_information( cli.queue)},
                        { "information-transaction", append_information( cli.transaction)},
                     };

                     information_t information;

                     auto dispatch = [ &mapping, &information]( auto& key)
                     {
                        auto is_key = [ &key]( auto& dispatch)
                        { 
                           return std::get< 0>( dispatch) == key;
                        };

                        if( auto found = algorithm::find_if( mapping, is_key))
                           std::get< 1>( *found)( information);
                        else
                           code::raise::error( code::casual::invalid_argument, "not a valid information context: ", key);
                     };
                     algorithm::for_each( managers, dispatch);

                     terminal::formatter::key::value().print( std::cout, information);
                  };

                  constexpr auto description = R"(collect general aggregated information about the domain
If no directives are provided all the _running_ managers are asked to provide information.
Otherwise, only the provided directives are used.

use auto-complete to aid valid directives.

valid directives:
* information-domain
* information-service
* information-queue
* information-transaction

)";

                  return common::argument::Option{
                     std::move( invoke),
                     complete,
                     { "--information"},
                     description};
               }

            } // information

            namespace version
            {
               auto options()
               {
                  auto invoke = []()
                  {
                     build::Version build_version = build::version();

                     std::vector< std::tuple< std::string, std::string>> version{
                        { "casual", build_version.casual},
                        { "commit", build_version.commit},
                        { "compiler", build_version.compiler}
                     };

                     terminal::formatter::key::value().print( std::cout, version);
                  };
                  return common::argument::Option{
                     std::move( invoke),
                     { "--version"},
                     "display version information"};
               }
            } // version

            namespace internal
            {
               namespace option
               {
                  namespace detail
                  {
                     auto fetch_handles( const std::vector< common::strong::process::id>& pids)
                     {
                        return algorithm::accumulate( pids, std::vector< process::Handle>{}, []( auto result, auto pid)
                        {
                           if( auto handle = communication::instance::fetch::handle( pid, communication::instance::fetch::Directive::direct))
                              result.push_back( handle);

                           return result;
                        });
                     }
                     
                  } // detail

                  auto state_dump()
                  {
                     auto invoke = []( std::vector< common::strong::process::id> pids)
                     {
                        auto send_message = []( auto&& handle)
                        {
                           communication::device::blocking::optional::send( handle.ipc, common::message::internal::dump::State{});
                        };

                        algorithm::for_each( detail::fetch_handles( pids), send_message);
                     };

                     return common::argument::Option{
                        std::move( invoke),
                        { "--state-dump"},
                        "dump state to casual.log for the provided pids, if the pid is able"
                     };
                  }

                  auto log_path()
                  {
                     auto invoke = []( std::filesystem::path path, std::vector< common::strong::process::id> pids)
                     {
                        common::message::internal::configure::Log message;
                        message.path = std::move( path);

                        auto send_message = [ &message]( auto&& handle)
                        {
                           communication::device::blocking::optional::send( handle.ipc, message);
                        };

                        algorithm::for_each( detail::fetch_handles( pids), send_message);
                     };

                     auto complete = []( auto&&, auto help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<path>", "<pid>"};
                        return { "<value>"};
                     };

                     return common::argument::Option{
                        std::move( invoke),
                        complete,
                        { "--log-path"},
                        R"(relocate the log-file for provided pids

Note: only works for 'servers' with a message pump)"
                     };
                  }

                  auto log_expression_inclusive()
                  {
                     auto invoke = []( std::string expression, std::vector< common::strong::process::id> pids)
                     {
                        common::message::internal::configure::Log message;
                        message.expression.inclusive = std::move( expression);

                        auto send_message = [ &message]( auto&& handle)
                        {
                           communication::device::blocking::optional::send( handle.ipc, message);
                        };

                        algorithm::for_each( detail::fetch_handles( pids), send_message);
                     };

                     auto complete = []( auto&&, auto help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<expression>", "<pid>"};
                        return { "<value>"};
                     };

                     return common::argument::Option{
                        std::move( invoke),
                        complete,
                        { "--log-expression-inclusive"},
                        R"(updates the _inclusive category filter_ (regex) for provided pids

Works the same as the `CASUAL_LOG` variable
Note: only works for 'servers' with a message pump)"
                     };
                  }
                  
               } // option


               auto group()
               {
                  return common::argument::Group{
                     [](){}, { "internal"}, "internal casual stuff for troubleshooting etc...",
                     option::state_dump(),
                     option::log_path(),
                     option::log_expression_inclusive(),
                  };
               }
               
            } // internal

            namespace pipe
            {
               namespace option
               {
                  auto human_sink()
                  {
                     auto invoke = []()
                     {
                        bool done = false;

                        auto handler = cli::message::dispatch::create(
                           // use all defaults, but force human readable
                           cli::pipe::forward::handle::defaults( true),
                           cli::pipe::handle::done( done)
                        );

                        // consume from casual-pipe
                        communication::stream::inbound::Device in{ std::cin};
                        common::message::dispatch::pump( 
                           cli::pipe::condition::done( done), 
                           handler, in);
                     };

                     return common::argument::Option{
                        std::move( invoke),
                        { "--human-sink"},
                        R"(INCUBATION - serialize casual pipe messages to human readable form, and in practice sink the message 

@attention INCUBATION - might change during, or in between minor version.

@attention: This is one way, there is no possibility to go from the output 
   back to _real_ pipe messages. Use only for stuff that are supposed to be discarded.)"
                     };
                  }
               } // option

               auto group()
               {
                  return common::argument::Group{
                     [](){}, { "pipe"}, "pipe related options",
                     option::human_sink()
                  };
               }
               
            } // pipe

         } // <unnamed>
      } // local

      struct CLI::Implementation
      {
         struct
         {
            casual::domain::manager::admin::cli domain;
            casual::service::manager::admin::cli service;
            queue::manager::admin::cli queue;
            casual::transaction::manager::admin::CLI transaction;
            gateway::manager::admin::cli gateway;
            casual::domain::discovery::admin::cli discovery;
            tools::service::call::cli service_call;
            tools::service::describe::cli describe;
            casual::buffer::admin::CLI buffer;
            configuration::admin::CLI configuration;
         } cli;

         auto parser() 
         {
            return argument::Parse{ R"(
casual administration CLI

To get more detailed help, use any of:
casual --help <option>
casual <option> --help
casual --help <option> <option> 

Where <option> is one of the listed below
)",
               local::information::option( cli),
               cli.domain.options(),
               cli.service.options(),
               cli.queue.options(),
               cli.transaction.options(),
               cli.gateway.options(),
               cli.discovery.options(),
               cli.service_call.options(),
               cli.describe.options(),
               cli.buffer.options(),
               cli.configuration.options(),
               local::pipe::group(),
               common::terminal::output::directive().options(),
               local::internal::group(),
               local::version::options(),
            };
         }
      };

      CLI::CLI() = default;
      CLI::~CLI() = default;

      argument::Parse CLI::parser() &
      {
         return m_implementation->parser();
      }

   } // administration
} // casual
