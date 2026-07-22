//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/common.h"
#include "tools/build/transform.h"
#include "tools/build/generate.h"

#include "configuration/build/model/load.h"
#include "configuration/system.h"

#include "casual/argument.h"
#include "common/exception/guard.h"

#include <fstream>
#include <iostream>

namespace casual
{
   using namespace common;
   namespace tools::build::server::generate
   {

      namespace local
      {
         namespace
         {
            struct Settings
            {
               struct
               {
                  std::string definition;
               } server;

               struct
               {
                  std::string system;
               } files;

               std::string output;
            };

            struct State
            {
               std::vector< model::Service> services;
               std::vector< model::Resource> resources;
            };


            namespace transform
            {
               auto state( const Settings& settings)
               {
                  Trace trace{ "tools::build::server::generate::local::transform::state"};

                  auto system = settings.files.system.empty() ?
                     configuration::system::get() : configuration::system::get( settings.files.system);

                  auto definition = configuration::build::model::load::server( settings.server.definition);

                  State result;

                  result.resources = build::transform::resources( 
                     definition,
                     {}, // no raw keys
                     system);

                  result.services = build::transform::services( 
                     definition,
                     {}, 
                     {});

                  return result;
               };

            } // transform

            void generate( const Settings& settings)
            {
               Trace trace{ "tools::build::server::generate::local::generate"};
               
               auto state = transform::state( settings);

               if( settings.output.empty())
                  build::generate::server( std::cout, state.resources, state.services);
               else
               {
                  std::ofstream out{ settings.output};
                  build::generate::server( out, state.resources, state.services);
               }
            }
            
            void main(int argc, const char** argv)
            {
               Settings settings;

               auto outcome = argument::parse( "generates a server 'main' source file", {
                  argument::Option( std::tie( settings.server.definition), {{ "-d", "--definition"}}, "path to server definition file")( argument::cardinality::one()),
                  argument::Option( std::tie( settings.output), {{ "-o", "--output"}}, "output file name - if not provided 'stdout' will be used"),
                  argument::Option( std::tie( settings.files.system), {{ "--system-configuration"}, { "-p", "--properties-file"}}, "path to system configuration file"),
               }, argc, argv);

               if( outcome != argument::Outcome::parsed)
                  return;

               local::generate( std::move( settings));
            }
         } // <unnamed>
      } // local

   } // tools::build::server::generate
} // casual


int main(int argc, const char** argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::tools::build::server::generate::local::main( argc, argv);
   });
}
