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
   namespace admin
   {

      int main( int argc, char **argv)
      {
         try
         {
            
            domain::manager::admin::cli domain;
            service::manager::admin::cli service;
            queue::manager::admin::cli queue;
            transaction::manager::admin::cli transaction;
            gateway::manager::admin::cli gateway;
            tools::service::call::cli service_call;
            tools::service::describe::cli describe;


            using namespace casual::common::argument;
            Parse parse{ R"(
casual administration CLI

To get more detailed help, use any of:
   casual --help <option>
   casual <option> --help
   casual --help <option> <option> 

Where <option> is one of the listed below
)", 
               domain.options(),
               service.options(),
               queue.options(),
               transaction.options(),
               gateway.options(),
               service_call.options(),
               describe.options(),
               common::terminal::output::directive().options(),
            };

            parse( argc, argv);

         }
         catch( ...)
         {
            return common::exception::handle( std::cerr);
         }

         return 0;
      }

   } // admin

} // casual


int main( int argc, char **argv)
{
   return casual::admin::main( argc, argv);
}


