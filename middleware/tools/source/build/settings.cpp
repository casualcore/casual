//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "tools/build/settings.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/file.h"

#include "configuration/system.h"
#include "configuration/model/load.h"

namespace casual
{
   namespace tools::build
   {
      namespace local
      {
         namespace
         {
            namespace option
            {
               auto build_directives( Settings& settings)
               {
                  return argument::Option( 
                     settings::split( settings.directives), 
                     { { "-f", "--build-directives"}, {"--link-directives"}}, 
                     "additional compile and link directives")( argument::cardinality::any());
               }

               auto no_defaults( Settings& settings)
               {
                  return argument::Option( 
                     argument::option::flag( settings.use_defaults), 
                     {{ "--no-defaults"}}, 
                     "do not add any default compiler/link directives\n\nuse --build-directives to add your own");
               }

               auto source_keep( settings::Source& source)
               {
                  return argument::Option( 
                     argument::option::flag( source.keep), 
                     {{ "-k", "--keep"}}, 
                     "keep the intermediate source file");
               }

               auto source_file( settings::Source& source)
               {
                  return argument::Option( 
                     std::tie( source.file), 
                     {{ "--source-file"}}, 
                     "name of the intermediate source file");
               }

               auto system_configuration( settings::System& system)
               {
                  return argument::Option( 
                     std::tie( system.globs), 
                     { { "--system-configuration"}, {"-p", "--properties-file"}}, 
                     R"(globs to system configuration files)");
               }

               auto only_generate( Settings& settings)
               {
                  return argument::Option( 
                     argument::option::flag( settings.only_generate), 
                     {{ "--only-generate"}}, 
                     R"(only generate the source file to stdout, do not build

Generates the source file to stdout and does not build the binary.
Useful for full build control. 
)");
               }

            } // option
            
         } // <unnamed>
      } // local

      void validate( const Settings& settings)
      {
         if( ! settings.use_defaults && ! settings.output.empty())
            common::code::raise::error( common::code::casual::invalid_argument, "output can't be used with 'no-defaults' - the 'output' has to be provided in a linker specific way");
      }


      namespace settings
      {
         std::vector< argument::Option> options( Settings& directive)
         {
            return {
               argument::Option( std::tie( directive.output), {{ "-o", "--output"}}, "name of binary to be built"),
               argument::Option( std::tie( directive.compiler), {{ "-c", "--compiler"}}, "compiler to use"),
               local::option::build_directives( directive),
               local::option::system_configuration( directive.system),
               local::option::no_defaults( directive),
               local::option::source_file( directive.source),
               local::option::source_keep( directive.source),
               local::option::only_generate( directive),
               argument::Option( argument::option::flag( directive.verbose), {{ "-v", "--verbose"}}, "verbose output")
            };
         }

         namespace resource::key
         {
            argument::Option option( settings::Resource& resource)
            {
               return argument::Option( 
                  argument::option::one::append( resource.keys), 
                  {{ "-r", "--resource-key"}},
                  "key of the resource");
            }
         } // resource::key

         configuration::model::system::Model system( const Settings& settings)
         {
            if( settings.system.globs.empty())
               return configuration::system::get();

            return configuration::model::load( common::file::find( settings.system.globs)).system;
         }

      } // settings

      
   } // tools::build
   
} // casual
