//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!
#include "tools/build/task.h"

#include "tools/common.h"

#include "common/algorithm.h"
#include "common/environment.h"

#include <iostream>

namespace casual
{
   using namespace common;
   namespace tools
   {
      namespace build
      {
         namespace local
         {
            namespace
            {
               
               namespace directive
               {
                  constexpr auto output = "-o";
                  constexpr auto link = "-l";

                  namespace path
                  {
                     constexpr auto include = "-I";
                     constexpr auto library = "-L";
                  } // path
               } // directive

            } // <unnamed>
         } // local

         void Directive::add( const std::vector< Resource>& resources)
         {
            for( auto& resource : resources)
            {
               algorithm::append_unique( resource.libraries, libraries);
               algorithm::append_unique( resource.paths.include, paths.include);
               algorithm::append_unique( resource.paths.library, paths.library);
            }
         }

         void validate( const Directive& settings)
         {

         }

         std::ostream& operator << ( std::ostream& out, const Directive& value)
         {
            return out << "{ compiler: " << value.compiler
               << ", directives: " << value.directives
               << ", libraries: " << value.libraries
               << ", output: " << value.output
               << ", verbose: " << value.verbose
               << ", paths: { include: " << value.paths.include
               << ", library: " << value.paths.library << '}'
               << '}';
         }


         void task( const std::string& input, const Directive& directive)
         {
            trace::Exit exit( "build task", directive.verbose);

            log::line( log, "input: ", input);
            log::line( log, "directive: ", directive);


            std::vector< std::string> arguments{ input, local::directive::output, directive.output};


            algorithm::transform( directive.libraries, arguments, []( auto& value){
               return local::directive::link + value;
            });

            algorithm::append( directive.directives, arguments);

            algorithm::transform( directive.paths.include, arguments, []( auto& value){
               return local::directive::path::include + value;
            });

            algorithm::transform( directive.paths.library, arguments, []( auto& value){
               return local::directive::path::library + value;
            });

            // Make sure we resolve environment stuff
            for( auto& argument : arguments)
               argument = common::environment::string( std::move( argument));

            if( directive.verbose)
               log::line( std::clog, directive.compiler, " ", common::string::join( arguments, " "));

            {
               trace::Exit log( "execute " + directive.compiler, directive.verbose);
               
               if( common::process::execute( directive.compiler, arguments) != 0)
                  throw common::exception::system::invalid::Argument{ "failed to compile"};
            }
         }

      } // build
   } // tools
} // casual