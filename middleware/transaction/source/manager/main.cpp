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

   try
   {
      casual::transaction::manager::Settings settings;
      {
         using namespace casual::common::argument;
         Parse parse{ "transaction manager",
            Option( std::tie( settings.log), { "-l", "--transaction-log"}, "path to transaction database log")
         };
         parse( argc, argv);
      }

      casual::transaction::Manager manager( std::move( settings));
      manager.start();

   }
   catch( ...)
   {
      return casual::common::exception::handle();

   }
   return 0;
}

