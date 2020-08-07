//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "gateway/manager/manager.h"


#include "common/argument.h"
#include "common/exception/handle.h"


namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         void main( int argc, char **argv)
         {
            // to provide --help, only for consistency
            casual::common::argument::Parse{
               R"(Responsible for interdomain communications.
)"
            }( argc, argv);

            Manager{}.start();
         }

      } // manager
   } // gateway
} // casual


int main( int argc, char **argv)
{
   return casual::common::exception::main::guard( [=]()
   {
       casual::gateway::manager::main( argc, argv);
   });
}

