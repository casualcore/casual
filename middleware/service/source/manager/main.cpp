//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "service/manager/manager.h"


#include "common/exception/handle.h"
#include "common/argument.h"


#include <iostream>

namespace casual
{
   using namespace common;

   namespace service
   {
      void main( int argc, char** argv)
      {
         manager::Settings settings;

         argument::Parse{ "casual-service-manager",
            argument::Option( std::tie( settings.forward), { "--forward"}, "path to the forward instance - mainly for unittest")
         }( argc, argv);

         Manager manager( std::move( settings));
         manager.start();
      }
   } // service

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::guard( [=]()
   {  
      casual::service::main( argc, argv);
   });
}
