//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/process.h"
#include "common/argument.h"
#include "common/environment.h"
#include "common/exception/guard.h"
#include "common/terminal.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "domain/manager/admin/cli.h"
#include "service/manager/admin/cli.h"
#include "queue/manager/admin/cli.h"
#include "transaction/manager/admin/cli.h"
#include "gateway/manager/admin/cli.h"
#include "casual/buffer/admin/cli.h"
#include "tools/service/call/cli.h"
#include "tools/service/describe/cli.h"



#include <string>
#include <vector>

#include <iostream>


namespace casual
{
   using namespace common;
   namespace admin
   {
      namespace local
      {
         namespace
         {
            namespace information
            {
               template< typename CLI>
               auto option( CLI& cli)
               {
                  auto complete = []( auto values, bool help) -> std::vector< std::string>
                  {
                     if( help)
                        return { "<value>"};
                     return { 
                        "information-domain",
                        "information-service",
                        "information-queue",
                        "information-transaction",
                     };
                  };

                  auto invoke = [&cli, complete]( std::vector< std::string> managers)
                  {
                     std::cout << "managers: " << managers << '\n';

                     // if not provided we collect from all
                     if( managers.empty())
                        managers = complete( 0, false);

                     std::cout << "managers: " << managers << '\n';

                     using information_t = decltype( cli.domain.information());

                     auto append_information = []( auto& cli)
                     {
                        return [&cli]( auto& information)
                        {
                           algorithm::append( cli.information(), information);
                        };
                     };

                     const std::vector< std::tuple< std::string, common::function< void( information_t&) const>>> mapping{
                        { "information-domain", append_information( cli.domain)},
                        { "information-service", append_information( cli.service)},
                        { "information-queue", append_information( cli.queue)},
                        { "information-transaction", append_information( cli.transaction)},
                     };

                     information_t information;

                     auto dispatch = [&mapping, &information]( auto& key)
                     {
                        if( auto found = algorithm::find_if( mapping, [&key]( auto& dispatch){ return std::get< 0>( dispatch) == key;}))
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
         } // <unnamed>
      } // local
      void main( int argc, char **argv)
      {
         struct
         {
            domain::manager::admin::cli domain;
            service::manager::admin::cli service;
            queue::manager::admin::cli queue;
            transaction::manager::admin::CLI transaction;
            gateway::manager::admin::cli gateway;
            tools::service::call::cli service_call;
            tools::service::describe::cli describe;
            casual::buffer::admin::CLI buffer;
         } cli;


         using namespace casual::common::argument;
         
         Parse{ R"(
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
            cli.service_call.options(),
            cli.describe.options(),
            cli.buffer.options(),
            common::terminal::output::directive().options(),
         }( argc, argv);

      }

   } // admin

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( std::cerr, [=]()
   {
      casual::admin::main( argc, argv);
   });
}


