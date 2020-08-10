//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/group.h"
#include "queue/common/log.h"

#include "common/argument.h"
#include "common/process.h"
#include "common/exception/handle.h"

namespace casual
{
   namespace queue
   {
      namespace group
      {
         namespace local
         {
            namespace
            {
               void main( int argc, char **argv)
               {
                  Settings settings;

                  using namespace common::argument;
                  Parse{ "queue group server",
                     Option( std::tie( settings.queuebase), { "-qb", "--queuebase"}, "path to this queue server persistent storage"),
                     Option( std::tie( settings.name), { "-n", "--name"}, "group name")
                  }( argc, argv);

                  common::log::line( verbose::log, "group settings: ", settings);

                  Server server( std::move( settings));
                  server.start();
               }

            } // <unnamed>
         } // local

      } // group
   } // queue
} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::guard( [&]()
   {
      casual::queue::group::local::main( argc, argv);
   });
}
