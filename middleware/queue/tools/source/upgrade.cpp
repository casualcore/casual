//! 
//! Copyright (c) 2010, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/group/queuebase/upgrade.h"

#include "common/exception/guard.h"
#include "common/algorithm.h"

#include "casual/argument.h"


namespace casual
{
   namespace queue::upgrade
   {
      namespace local
      {
         namespace
         {
            void main( int argc, const char** argv)
            {
               std::vector< std::filesystem::path> files;

               argument::parse( "upgrades queue-base files to latest version", {
                  argument::Option{ argument::option::one::many( files), {{ "-f", "--files"}}, "queue-base files to upgrade" }( argument::cardinality::one())
               }, argc, argv);
               
               common::algorithm::for_each( files, &queue::group::upgrade::queuebase);
            }
         } // <unnamed>
      } // local

      
   } // queue::update
} // casual


int main( int argc, const char** argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::queue::upgrade::local::main( argc, argv);
   });
}
