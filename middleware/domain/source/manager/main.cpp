//!
//! casual
//!


#include "domain/manager/manager.h"


#include "common/error.h"
#include "common/arguments.h"


namespace casual
{
   namespace domain
   {
      namespace manager
      {

         int main( int argc, char** argv)
         {
            try
            {

               Settings settings;

               {
                  common::Arguments parser{ {
                     common::argument::directive( common::argument::cardinality::Any{}, {"-c", "--configuration-files"}, "domain configuration files", settings.configurationfiles),
                     common::argument::directive( {"-b", "--boot"}, "boots the domain based on the stored configuration", settings.boot),
                     common::argument::directive( {"--bare"}, "do not boot mandatory (broker, TM), mostly for unittest", settings.bare)
                     }};


                  parser.parse( argc, argv);
               }

               Manager domain( std::move( settings));
               domain.start();

            }
            catch( ...)
            {
               return casual::common::error::handler();

            }
            return 0;
         }

      } // manager
   } // domain
} // casual


int main( int argc, char** argv)
{
   return casual::domain::manager::main( argc, argv);
}
