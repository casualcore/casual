//!
//! casual_broker_main.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "domain/domain.h"


#include "common/error.h"
#include "common/arguments.h"


namespace casual
{
   namespace domain
   {
      int main( int argc, char** argv)
      {
         try
         {

            Settings settings;

            {
               common::Arguments parser{
                  { common::argument::directive( {"-c", "--configuration-files"}, "domain configuration files", settings.configurationfiles)}};


               parser.parse( argc, argv);
            }

            Domain domain( std::move( settings));
            domain.start();

         }
         catch( ...)
         {
            return casual::common::error::handler();

         }
         return 0;
      }

   } // domain
} // casual


int main( int argc, char** argv)
{
   return casual::domain::main( argc, argv);
}
