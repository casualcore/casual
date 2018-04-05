//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/group.h"


#include "common/argument.h"
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
         using namespace casual::common::argument;
         Parse parse{ "queue group server",
            Option( std::tie( settings.queuebase), { "-qb", "--queuebase"}, "path to this queue server persistent storage"),
            Option( std::tie( settings.name), { "-n", "--name"}, "group name")
         };
         parse( argc, argv);
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
