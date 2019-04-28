//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/manager.h"
#include "common/argument.h"
#include "common/exception/handle.h"


namespace casual
{

   namespace queue
   {
      namespace manager
      {
         void main( int argc, char **argv)
         {
            Settings settings;
            {
               using namespace casual::common::argument;
               Parse{ R"(
Manages casual queue, the provided queue functionality.
)",
                  Option( std::tie( settings.group_executable), {"-g", "--group-executable"}, "path to casual-queue-group only (?) for unittest")
               }( argc, argv);
            }

            queue::Manager manager( std::move( settings));
            manager.start();
         }
         
      } // manager
   } // queue
} // casual

int main( int argc, char **argv)
{
   try
   {
      casual::queue::manager::main( argc, argv);
      return 0;
   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }
}


