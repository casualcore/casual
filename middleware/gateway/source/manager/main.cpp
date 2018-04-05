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

         int main( int argc, char **argv)
         {

            try
            {
               Settings settings;
               {
                  casual::common::argument::Parse parse{
                     R"(
Responsible for interdomain communications.
)"
                  };
                  parse( argc, argv);
               }

               Manager manager{ std::move( settings)};
               manager.start();

            }
            catch( ...)
            {
               return casual::common::exception::handle();
            }
            return 0;
         }

      } // manager
   } // gateway

} // casual


int main( int argc, char **argv)
{
   return casual::gateway::manager::main( argc, argv);
}

