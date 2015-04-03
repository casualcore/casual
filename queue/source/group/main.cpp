//!
//! queue_main.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include "queue/group/group.h"


#include "common/arguments.h"
#include "common/process.h"
#include "common/error.h"

namespace casual
{




} // casual

int main( int argc, char **argv)
{

   try
   {

      casual::queue::group::Settings settings;

      {
         casual::common::Arguments parser;
         parser.add(
               casual::common::argument::directive( { "-qb", "--queuebase"}, "path to this queue server persistent storage", settings.queuebase),
               casual::common::argument::directive( { "-n", "--name"}, "group name", settings.name)
         );

         parser.parse( argc, argv);
         casual::common::process::path( parser.processName());
      }

      casual::queue::group::Server server( std::move( settings));
      server.start();

   }
   catch( ...)
   {
      return casual::common::error::handler();
   }


   return 0;
}
