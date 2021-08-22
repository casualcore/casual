//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!
#include "tools/build/task.h"

#include "tools/common.h"

#include "common/algorithm.h"
#include "common/environment/expand.h"
#include "common/process.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

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

         void validate( const Directive& settings)
         {
            if( ! settings.use_defaults && ! settings.output.empty())
               code::raise::error( code::casual::invalid_argument, "output can't be used with 'no-defaults' - the 'output' has to be provided in a linker specific way");
         }


         void task( const std::filesystem::path& input, const Directive& directive)
         {
            trace::Exit exit( "build task", directive.verbose);

            validate( directive);

            log::line( log, "input: ", input);
            log::line( log, "directive: ", directive);


            auto arguments = [&]() ->  std::vector< std::string>
            {
               if( directive.use_defaults)
                   return { input, local::directive::output, directive.output};
               else
                  return { input};
            }();

            if( directive.use_defaults) 
            {
               algorithm::transform( directive.libraries, arguments, []( auto& value)
               {
                  return local::directive::link + value;
               });
            }

            algorithm::append( directive.directives, arguments);

            if( directive.use_defaults) 
            {  
               algorithm::transform( directive.paths.include, arguments, []( auto& value)
               {
                  return local::directive::path::include + value;
               });

               algorithm::transform( directive.paths.library, arguments, []( auto& value)
               {
                  return local::directive::path::library + value;
               });
            }


            // Make sure we resolve environment stuff
            for( auto& argument : arguments)
               argument = common::environment::expand( std::move( argument));

            if( directive.verbose)
               log::line( std::clog, directive.compiler, " ", common::string::join( arguments, " "));

            {
               trace::Exit log( "execute " + directive.compiler, directive.verbose);
               
               if( common::process::execute( directive.compiler, arguments) != 0)
                  code::raise::error( code::casual::invalid_argument, "failed to compile");
            }
         }

      } // build
   } // tools
} // casual