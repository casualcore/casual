//!
//! main.cpp
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
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

         common::Arguments parser;

         parser.add(
               common::argument::directive( {"-c", "--configuration"}, "queue configuration file", settings.configuration)
         );

         parser.parse( argc, argv);

         common::process::path( parser.processName());
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


