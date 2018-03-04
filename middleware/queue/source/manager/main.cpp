//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/manager.h"
#include "common/arguments.h"
#include "common/exception/handle.h"

using namespace casual;

int main( int argc, char **argv)
{

   try
   {

      queue::manager::Settings settings;

      {

         common::Arguments parser{
            R"(
Manages casual queue, the provided queue functionality.
)",
            {
               common::argument::directive( {"-g", "--group-executable"}, "path to casual-queue-group only (?) for unittest", settings.group_executable)
         }};

         parser.parse( argc, argv);
      }

      queue::Broker broker( settings);

      broker.start();

   }
   catch( const common::exception::signal::Terminate&)
   {
      return 0;
   }
   catch( ...)
   {
      return casual::common::exception::handle();

   }
   return 0;
}


