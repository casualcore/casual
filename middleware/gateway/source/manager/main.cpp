//!
//! casual
//!

#include "gateway/manager/manager.h"


#include "common/arguments.h"
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
                  casual::common::Arguments parser{
                     R"(
Responsible for interdomain communications.
)", {}
                  };
                  parser.parse( argc, argv);
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

