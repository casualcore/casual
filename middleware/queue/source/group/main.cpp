//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/group.h"


#include "common/arguments.h"
#include "common/process.h"
#include "common/exception/handle.h"

namespace casual
{




} // casual

int main( int argc, char **argv)
{

   try
   {

      casual::queue::group::Settings settings;

      {
         casual::common::Arguments parser{ {
               casual::common::argument::directive( { "-qb", "--queuebase"}, "path to this queue server persistent storage", settings.queuebase),
               casual::common::argument::directive( { "-n", "--name"}, "group name", settings.name)
         }};

         parser.parse( argc, argv);
      }

      casual::queue::group::Server server( std::move( settings));
      return server.start();

   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }


   return 0;
}
