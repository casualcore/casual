//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/argument.h"
#include "common/process.h"
#include "common/exception/handle.h"

#include "common/environment.h"

#include "transaction/manager/manager.h"


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [&]()
   {
      casual::transaction::manager::Settings settings;
      {
         using namespace casual::common::argument;
         
         Parse{ "transaction manager",
            Option( std::tie( settings.log), { "-l", "--transaction-log"}, "path to transaction database log")
         }( argc, argv);
      }

      casual::transaction::Manager manager( std::move( settings));
      manager.start();
   });
}

