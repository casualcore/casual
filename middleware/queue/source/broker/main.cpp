//!
//! casual
//!

#include "queue/broker/broker.h"
#include "common/arguments.h"
#include "common/error.h"

using namespace casual;

int main( int argc, char **argv)
{

   try
   {

      queue::broker::Settings settings;

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
      return casual::common::error::handler();

   }
   return 0;
}


