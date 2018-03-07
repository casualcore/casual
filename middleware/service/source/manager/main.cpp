//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "service/manager/manager.h"


#include "common/exception/handle.h"
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
            return casual::common::exception::handle();

         }
         return 0;
      }
   } // service

} // casual


int main( int argc, char** argv)
{
   return casual::service::main( argc, argv);
}
