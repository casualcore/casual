//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "http/outbound/manager.h"

#include "common/argument.h"
#include "common/exception/handle.h"

namespace casual
{
   namespace http
   {
      namespace outbound
      {
         void main( int argc, char **argv)
         {
            manager::Settings settings;
            {
               using namespace casual::common::argument;
               Parse parse{ "http outbound",
                  Option( std::tie( settings.configurations), { "--configuration-files"}, "[deprecated]"),
                  Option( std::tie( settings.configurations), { "--configuration"}, "configuration files")
               };

               parse( argc, argv);
            }

            Manager manager{ std::move( settings)};
            manager.run();
         }
      } // outbound
   } // http

} // casual

int main( int argc, char **argv)
{
   try
   {
      casual::http::outbound::main( argc, argv);
   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }
   return 0;
}



