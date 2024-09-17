//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/argument.h"
#include "common/log.h"
#include "common/exception/guard.h"


namespace casual
{
   namespace file::manager
   {
      namespace local
      {
         namespace
         {
            struct Settings
            {
            };

            using Trace = common::Trace;

            void start( Settings settings)
            {
               Trace trace( "file::manager::local::start");

            }

            void main( int argc, char **argv)
            {
               Settings settings;

               {
                  common::argument::Parse{ R"(Manages casual file, the provided file functionality)",
                  }( argc, argv);
               }

               start( std::move( settings));
            }

         } //
      } // local
         
   } // file::manager
} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::file::manager::local::main( argc, argv);
   });
}


