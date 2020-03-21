//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/process.h"
#include "common/argument.h"
#include "common/environment.h"
#include "common/exception/handle.h"
#include "common/terminal.h"


#include "domain/manager/admin/cli.h"
#include "service/manager/admin/cli.h"
#include "queue/manager/admin/cli.h"
#include "transaction/manager/admin/cli.h"
#include "gateway/manager/admin/cli.h"
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
               void invoke( CLI& cli)
               {
                  auto accumulate = []( auto& cli)
                  {
                     auto result = cli.domain.information();

                     return result;
                  };


                  auto get_first = []( auto& pair) -> const std::string& { return std::get< 0>( pair);};
                  auto get_second = []( auto& pair) -> const std::string& { return std::get< 1>( pair);};

                  auto formatter = terminal::format::formatter< std::tuple< std::string, std::string>>::construct(
                     terminal::format::column( "category", get_first, terminal::color::yellow, terminal::format::Align::left),
                     terminal::format::column( "value", get_second, terminal::color::no_color, terminal::format::Align::left)
                  );

                  auto information = accumulate( cli);

                  formatter.print( std::cout, information);


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
            transaction::manager::admin::cli transaction;
            gateway::manager::admin::cli gateway;
            tools::service::call::cli service_call;
            tools::service::describe::cli describe;
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
            Option{ [&cli](){ local::information::invoke( cli);}, { "--information"}, "collect general aggregated information about the domain"},
            cli.domain.options(),
            cli.service.options(),
            cli.queue.options(),
            cli.transaction.options(),
            cli.gateway.options(),
            cli.service_call.options(),
            cli.describe.options(),
            common::terminal::output::directive().options(),
         }( argc, argv);

      }

   } // admin

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::guard( std::cerr, [=]()
   {
      casual::admin::main( argc, argv);
   });
}


