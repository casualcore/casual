//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "tools/build/setting.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

namespace casual
{
   namespace tools::build::setting
   {
      namespace local
      {
         namespace
         {
            namespace option
            {
               auto build_directives( setting::Mandatory& directive)
               {
                  return argument::Option( 
                     mandatory::split( directive.directives), 
                     { { "-f", "--build-directives"}, {"--link-directives"}}, 
                     "additional compile and link directives")( argument::cardinality::any());
               }

               auto no_defaults( setting::Mandatory& directive)
               {
                  return argument::Option( 
                     argument::option::flag( directive.use_defaults), 
                     {{ "--no-defaults"}}, 
                     "do not add any default compiler/link directives\n\nuse --build-directives to add your own");
               }

               auto source_keep( mandatory::Source& source)
               {
                  return argument::Option( argument::option::flag( source.keep), {{ "-k", "--keep"}}, "keep the intermediate source file");
               }

               auto source_file( mandatory::Source& source)
               {
                  return argument::Option( std::tie( source.file), {{ "--source-file"}}, "name of the intermediate source file");
               }

               auto system_configuration( mandatory::System& system)
               {
                  return argument::Option( 
                     std::tie( system.configuration), 
                     { { "--system-configuration"}, {"-p", "--properties-file"}}, 
                     "path to system configuration file");
               }

            } // option
            
         } // <unnamed>
      } // local

      void validate( const Mandatory& settings)
      {
         if( ! settings.use_defaults && ! settings.output.empty())
            common::code::raise::error( common::code::casual::invalid_argument, "output can't be used with 'no-defaults' - the 'output' has to be provided in a linker specific way");
      }
      namespace mandatory
      {
         std::vector< argument::Option> options( Mandatory& directive)
         {
            return {
               argument::Option( std::tie( directive.output), {{ "-o", "--output"}}, "name of binary to be built"),
               argument::Option( std::tie( directive.compiler), {{ "-c", "--compiler"}}, "compiler to use"),
               local::option::build_directives( directive),
               local::option::system_configuration( directive.system),
               local::option::no_defaults( directive),
               local::option::source_file( directive.source),
               local::option::source_keep( directive.source),
               argument::Option( argument::option::flag( directive.verbose), {{ "-v", "--verbose"}}, "verbose output")
            };
         }

      } // mandatory

      
   } // tools::build::setting
   
} // casual
