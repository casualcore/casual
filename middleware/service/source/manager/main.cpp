//!
//! casual
//!


#include "service/manager/manager.h"


#include "common/error.h"
#include "common/arguments.h"


#include <iostream>

namespace casual
{
   using namespace common;

   namespace service
   {
      int main( int argc, char** argv)
      {
         try
         {

            manager::Settings settings;

            {
               Arguments parser{ "casual-service-manager",
                  {
                        argument::directive( { "--forward"}, "path to the forward instance - mainly for unittest", settings.forward)
                  }

               };

               parser.parse( argc, argv);

            }

            Manager manager( std::move( settings));
            manager.start();

         }
         catch( ...)
         {
            return casual::common::error::handler();

         }
         return 0;
      }
   } // service

} // casual


int main( int argc, char** argv)
{
   return casual::service::main( argc, argv);
}
