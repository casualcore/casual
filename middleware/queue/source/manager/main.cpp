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
                  Option( std::tie( settings.executable.path), { "--executable-path"}, "path for unittest"),
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
   return casual::common::exception::main::guard( [=]()
   {
      casual::queue::manager::main( argc, argv);
   });
}


