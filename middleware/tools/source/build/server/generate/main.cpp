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

#include "common/argument.h"
#include "common/exception/guard.h"

#include <fstream>

namespace casual
{
   using namespace common;
   namespace tools
   {
      namespace build
      {
         namespace server
         {
            namespace generate
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
                           build::generate::server( std::cout, state.resources, state.services);
                        }
                     }
                     
                     constexpr auto description = R"(Generates a server 'main' source file

)";

                     void main(int argc, char** argv)
                     {
                        Trace trace{ "tools::build::server::generate::local::main"};

                        Settings settings;

                        using namespace casual::common::argument;
                        Parse{ description,
                           Option( std::tie( settings.server.definition), { "-d", "--definition"}, "path to server definition file")( argument::cardinality::one{}),
                           Option( std::tie( settings.server.definition), { "-o", "--output"}, "output file name - if not provided 'stdout' will be used"),
                           Option( std::tie( settings.files.system), argument::option::keys( { "--system-configuration"}, {"-p", "--properties-file"}), "path to system configuration file"),
                        }( argc, argv);

                        local::generate( std::move( settings));
                     }
                  } // <unnamed>
               } // local
            } // generate
         } // server

      } // build
   } // tools 
} // casual


int main(int argc, char** argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::tools::build::server::generate::local::main( argc, argv);
   });
}
