//!
//! casual 
//!

#include "common/arguments.h"


namespace casual
{
   using namespace common;

   namespace sf
   {
      namespace service
      {
         namespace describe
         {


            struct Settings
            {
               std::string protocol;
               std::vector< std::string> services;

            };

         } // describe


         int main( int argc, char **argv)
         {
            describe::Settings settings;

            try
            {
               Arguments arguments{ "Describes a casual service",
                  {
                     argument::directive( {"-s", "--services"}, "services to describe ", settings.services),
                     argument::directive( {"-f", "--format"}, "format to print [json,yaml,xml,ini,debug...] ", settings.protocol)
                  }};

               arguments.parse( argc, argv);
            }
            catch( ...)
            {
               return common::error::handler();
            }

            return 0;
         }

      } // service
   } // sf
} // casual


int main( int argc, char **argv)
{
   return casual::sf::service::main( argc, argv);
}
