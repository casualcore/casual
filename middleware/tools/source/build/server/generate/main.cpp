//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/common.h"
#include "tools/build/transform.h"
#include "tools/build/generate.h"

#include "configuration/build/server.h"
#include "configuration/resource/property.h"

#include "common/argument.h"
#include "common/exception/handle.h"

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
                           std::string properties;
                        } resource;

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

                           auto properties = settings.resource.properties.empty() ?
                              configuration::resource::property::get() : configuration::resource::property::get( settings.resource.properties);

                           auto definition = configuration::build::server::get( settings.server.definition);

                           State result;

                           result.resources = build::transform::resources( 
                              definition.resources,
                              {}, // no raw keys
                              properties);

                           result.services = build::transform::services( 
                              definition.services,
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
                           Option( std::tie( settings.resource.properties), {"-p", "--resource-properties"}, "path to resource properties file"),
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
   return casual::common::exception::guard( std::cerr, [=]()
   {
      casual::tools::build::server::generate::local::main( argc, argv);
   });
}
